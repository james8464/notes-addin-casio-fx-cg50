// Interface: *GUI*, main.cpp, console.*, *Provider*
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
int khicas_addins_menu(GIAC_CONTEXT);
extern "C" {
#include <fxcg/rtc.h>
  extern int execution_in_progress_py;
  extern volatile int ctrl_c_py;
  void set_abort_py();
  void clear_abort_py();
}
extern "C" int execution_in_progress_py=0;
extern "C" volatile int ctrl_c_py=0;
extern "C" void set_abort_py(){}
extern "C" void clear_abort_py(){}
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>
#include <gint/kmalloc.h>

#include "console.h"
  //#include "memmgr.h"
#include "catalogGUI.hpp"
#include "fileGUI.hpp"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"
#include "kdisplay.h"
#include "input_lexer.h"
#include "input_parser.h"
#include "main.h"
#include "cascas_working.h"
#define GIAC_HISTORY_SIZE 8
#define GIAC_HISTORY_MAX_TAILLE 32

using namespace giac;
int esc_flag=0;
int pythonjs_stack_size=32*1024,pythonjs_heap_size=256*1024;
char * pythonjs_static_heap=0;
int select_item(const char ** ptr,const char * title,bool askfor1){
  int nitems=0;
  for (const char ** p=ptr;*p;++p)
    ++nitems;
  if (nitems==0 || nitems>=256)
    return -1;
  if (!askfor1 && nitems==1)
    return 0;
  MenuItem smallmenuitems[nitems];
  for (int i=0;i<nitems;++i){
    smallmenuitems[i].text=(char *) ptr[i];
  }
  Menu smallmenu;
  smallmenu.numitems=nitems; 
  smallmenu.items=smallmenuitems;
  smallmenu.height=8;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char*) title;
  //MsgBoxPush(5);
  int sres = doMenu(&smallmenu);
  //MsgBoxPop();
  if (sres!=MENU_RETURN_SELECTION && sres!=KEY_CTRL_EXE)
    return -1;
  return smallmenu.selection-1;
}

#ifndef MICROPY_LIB
int select_interpreter(){
  const char * choix[]={"CAS mode","CAS mode","CAS mode",0};
  return select_item(choix,"Syntax",false);
}

#else
int select_interpreter(){
  const char * choix[]={"CAS mode","CAS mode","CAS mode","CAS mode",0,"CAS mode"};
  return select_item(choix,"Syntax",false);
}


char * python_heap=0;
int exproffset = 0;
int run_startup_script_again=0;
//extern void set_rnd_seed(int);
int custom_key_to_handle;
int custom_key_to_handle_modifier;
int has_drawn_graph = 0;


extern char* outputRedirectBuffer;
extern int remainingBytesInRedirect;
extern "C" int mp_token(const char * line);

void python_free(){
  if (!python_heap) return;
  mp_deinit();
  if (!pythonjs_static_heap){
    if ( ((size_t) python_heap)>1)
      free(python_heap);
  }
  python_heap=0;
}

int python_init(int stack_size,int heap_size){
#if 1 // defined NUMWORKS
  python_free();
  python_heap=micropy_init(stack_size,heap_size);
  if ( ((int) python_heap)>1)
    cout << "init stack=" << stack_size << " heap=" << heap_size << '\n';
  // cout << "heap " << (int) python_heap << "\n";
  if (!python_heap)
    return 0;
#endif
  return 1;
}

int micropy_ck_eval(const char *line){
#if 1 // def NUMWORKS
  if (python_heap && line[0]==0)
    return 1;
  if (!python_heap){
    python_init(pythonjs_stack_size,pythonjs_heap_size);
  }
  if (!python_heap){
    xcas_python_eval=0; python_compat(0,contextptr);
    return RAND_MAX;
  }
#endif
  ctrl_c_py=0;
  execution_in_progress_py = 1;
  int res= micropy_eval(line);
  execution_in_progress_py = 0;
  if (ctrl_c_py & 1){
    while (confirm4("Interrupted","F1/F6: ok",true,120)==-1)
      ; // insure ON has been removed from keyboard buffer
  }  
  //while (1) { int key; ck_getkey(&key); if (key==KEY_CTRL_EXIT) break; }
  return res;
}


void console_output(const char * s,int l){
  char buf[l+1];
  strncpy(buf,s,l);
  buf[l]=0;
  dConsolePut(buf);
}

const char * console_input(const char * msg1,const char * msg2,bool numeric,int ypos){
  string str;
  if (!inputline(msg1,msg2,str,numeric,ypos))
    return 0;
  char * ptr=strdup(str.c_str());
  return ptr;
}

int c_yshift=24;

void c_draw_rectangle(int x,int y,int w,int h,int c){
  giac::freeze=true;
  y += c_yshift;
  draw_line(x,y,x+w,y,c);
  draw_line(x+w,y,x+w,y+h,c);
  draw_line(x,y+h,x+w,y+h,c);
  draw_line(x,y,x,y+h,c);
}
void c_draw_line(int x0,int y0,int x1,int y1,int c){
  giac::freeze=true;
  y0 += c_yshift;
  y1 += c_yshift;
  draw_line(x0,y0,x1,y1,c);
}
void c_draw_circle(int xc,int yc,int r,int color,bool q1,bool q2,bool q3,bool q4){
  giac::freeze=true;
  yc += c_yshift;
  draw_circle(xc,yc,r,color,q1,q2,q3,q4);
}
void c_draw_filled_circle(int xc,int yc,int r,int color,bool left,bool right){
  giac::freeze=true;
  yc += c_yshift;
  draw_filled_circle(xc,yc,r,color,left,right);
}
void c_convert(int *x,int*y,vector< vector<int> > & v){
  for (int i=0;i<v.size();++i,++x,++y){
    v[i].push_back(*x);
    v[i].push_back(*y+c_yshift);
  }
}
void c_draw_polygon(int * x,int *y ,int n,int color){
  giac::freeze=true;
  vector< vector<int> > v(n);
  c_convert(x,y,v);
  draw_polygon(v,color);
}
void c_draw_filled_polygon(int * x,int *y, int n,int xmin,int xmax,int ymin,int ymax,int color){
  giac::freeze=true;
  vector< vector<int> > v(n);
  c_convert(x,y,v);
  draw_filled_polygon(v,xmin,xmax,ymin,ymax,color);
}
void c_draw_arc(int xc,int yc,int rx,int ry,int color,double theta1, double theta2){
  giac::freeze=true;
  yc += c_yshift;
  draw_arc(xc,yc,rx,ry,color,theta1,theta2);
}
void c_draw_filled_arc(int x,int y,int rx,int ry,int theta1_deg,int theta2_deg,int color,int xmin,int xmax,int ymin,int ymax,bool segment){
  giac::freeze=true;
  y += c_yshift;
  draw_filled_arc(x,y,rx,ry,theta1_deg,theta2_deg,color,xmin,xmax,ymin,ymax,segment);
}
void c_set_pixel(int x,int y,int c){
  giac::freeze=true;
  y += c_yshift;
  os_set_pixel(x,y,c);
}
void c_fill_rect(int x,int y,int w,int h,int c){
  y += c_yshift;
  giac::freeze=true;
  if (w<0){
    w=-w;
    x -= w; 
  }
  if (h<0){
    h=-h;
    y -= h; 
  }
  if (x<0){ w+=x; x=0;}
  if (y<0){ h+=y; y=0;}
  os_fill_rect(x,y,w,h,c);
}
int c_draw_string(int x,int y,int c,int bg,const char * s,bool fake){
  giac::freeze=true;
  y += c_yshift;
  return os_draw_string(x,y,c,bg,s,fake);
}
int c_draw_string_small(int x,int y,int c,int bg,const char * s,bool fake){
  giac::freeze=true;
  y += c_yshift;
  return os_draw_string_small(x,y,c,bg,s,fake);
}
int c_draw_string_medium(int x,int y,int c,int bg,const char * s,bool fake){
  giac::freeze=true;
  y += c_yshift;
  return os_draw_string_medium(x,y,c,bg,s,fake);
}

ulonglong double2gen(double d){
  giac::gen g(d);
  return *(ulonglong *) &g;
}

ulonglong int2gen(int d){
  giac::gen g(d);
  return *(ulonglong *) &g;
}

void turtle_freeze(){
  freezeturtle=true;
}

void doubleptr2matrice(double * x,int n,int m,giac::matrice & M){
  M.resize(n);
  for (int i=0;i<n;++i){
    M[i]=giac::vecteur(m);
    giac::vecteur & w=*M[i]._VECTptr;
    for (int j=0;j<m;++j){
      w[j]=*x;
      ++x;
    }
  }
}

// x must have enough space!
bool matrice2doubleptr(const giac::matrice &M,double *x){
  int n=M.size();
  if (n==0 || M.front().type!=giac::_VECT)
    return false;
  int m=M.front()._VECTptr->size();
  for (int i=0;i<n;++i){
    if (M[i].type!=giac::_VECT || M[i]._VECTptr->size()!=m)
      return false;
    giac::vecteur & w=*M[i]._VECTptr;
    for (int j=0;j<m;++j){
      giac::gen g =giac::evalf_double(w[j],1,giac::context0);
      if (g.type!=giac::_DOUBLE_)
	return false;
      *x=g._DOUBLE_val;
      ++x;
    }
  }
  return true;
}

bool r_inv(double * x,int n){
  giac::matrice M(n);
  doubleptr2matrice(x,n,n,M);
  M=giac::minv(M,giac::context0);
  return matrice2doubleptr(M,x);
}


bool r_rref(double * x,int n,int m){
  giac::matrice M(n);
  doubleptr2matrice(x,n,m,M);
  giac::gen g=giac::_rref(M,giac::context0);
  if (g.type!=giac::_VECT)
    return false;
  return matrice2doubleptr(*g._VECTptr,x);
}

double r_det(double *x,int n){
  giac::matrice M(n);
  doubleptr2matrice(x,n,n,M);
  giac::gen g=giac::mdet(M,giac::context0);
  g=giac::evalf_double(g,1,giac::context0);
  double d=1.0,e=1.0;
  if (g.type!=_DOUBLE_)
    return 0.0/(d-e);
  return g._DOUBLE_val;
}

void c_complexptr2matrice(c_complex * x,int n,int m,giac::matrice & M){
  M.resize(n);
  for (int i=0;i<n;++i){
    if (m==0){
      M[i]=gen(x->r,x->i);
      ++x;
      continue;
    }
    M[i]=giac::vecteur(m);
    giac::vecteur & w=*M[i]._VECTptr;
    for (int j=0;j<m;++j){
      w[j]=gen(x->r,x->i);
      ++x;
    }
  }
}

c_complex gen2c_complex(giac::gen & g){
  double d=1.0,e=1.0;
  c_complex c={0,0};
  if (g.type!=giac::_DOUBLE_ && g.type!=giac::_CPLX)
    c.r=c.i=0.0/(d-e);
  else {
    if (g.type==giac::_DOUBLE_)
      c.r=g._DOUBLE_val;
    else {
      if (g.subtype!=3)
	c.r=c.i=0.0/(d-e);
      c.r=g._CPLXptr->_DOUBLE_val;
      c.i=(g._CPLXptr+1)->_DOUBLE_val;
    }
  }
  return c;
}

// x must have enough space!
bool matrice2c_complexptr(const giac::matrice &M,c_complex *x){
  int n=M.size();
  if (n==0)
    return false;
  if (M.front().type!=giac::_VECT){
    for (int i=0;i<n;++i){
      giac::gen g =giac::evalf_double(M[i],1,giac::context0);
      if (g.type!=giac::_DOUBLE_ && g.type!=giac::_CPLX)
	return false;
      *x=gen2c_complex(g);
      ++x;
    }
    return true;
  }
  int m=M.front()._VECTptr->size();
  for (int i=0;i<n;++i){
    if (M[i].type!=giac::_VECT || M[i]._VECTptr->size()!=m)
      return false;
    giac::vecteur & w=*M[i]._VECTptr;
    for (int j=0;j<m;++j){
      giac::gen g =giac::evalf_double(w[j],1,giac::context0);
      if (g.type!=giac::_DOUBLE_ && g.type!=giac::_CPLX)
	return false;
      *x=gen2c_complex(g);
      ++x;
    }
  }
  return true;
}

bool c_inv(c_complex * x,int n){
  giac::matrice M(n);
  c_complexptr2matrice(x,n,n,M);
  M=giac::minv(M,giac::context0);
  return matrice2c_complexptr(M,x);
}

bool c_proot(c_complex * x,int n){
  giac::matrice M(n);
  c_complexptr2matrice(x,n,0,M);
  M=giac::proot(M);
  return matrice2c_complexptr(M,x);
}

bool c_pcoeff(c_complex * x,int n){
  giac::matrice M(n);
  c_complexptr2matrice(x,n,0,M);
  M=giac::pcoeff(M);
  return matrice2c_complexptr(M,x);
}

bool c_fft(c_complex * x,int n,bool inverse){
#if 1
  complex<double> * X=(complex<double> *) x;
  double theta=2*M_PI/n;
  if (!inverse)
    theta=-theta;
  //fft2(X,n,theta); // FIXME
  if (inverse){
    for (int i=0;i<n;++i)
      X[i]=X[i]/double(n);
  }
  return true;
#else
  giac::matrice M(n);
  c_complexptr2matrice(x,n,0,M);
  gen g=inverse?giac::_ifft(M,giac::context0):giac::_fft(M,giac::context0);
  if (g.type!=_VECT)
    return false;
  return matrice2c_complexptr(*g._VECTptr,x);
#endif
}

bool c_egv(c_complex * x,int n){
  giac::matrice M(n);
  c_complexptr2matrice(x,n,n,M);
  gen g=giac::_egv(M,giac::context0);
  if (!ckmatrix(g))
    return false;
  return matrice2c_complexptr(*g._VECTptr,x);
}

bool c_eig(c_complex * x,c_complex * d,int n){
  giac::matrice M(n);
  c_complexptr2matrice(x,n,n,M);
  gen g=giac::_jordan(M,giac::context0);
  if (g.type!=_VECT || g._VECTptr->size()!=2 || !ckmatrix(g[0]) || !ckmatrix(g[1]))
    return false;
  return matrice2c_complexptr(*g[0]._VECTptr,x) && matrice2c_complexptr(*g[1]._VECTptr,d);
}

bool c_rref(c_complex * x,int n,int m){
  giac::matrice M(n);
  c_complexptr2matrice(x,n,m,M);
  giac::gen g=giac::_rref(M,giac::context0);
  if (g.type!=giac::_VECT)
    return false;
  return matrice2c_complexptr(*g._VECTptr,x);
}

c_complex c_det(c_complex *x,int n){
  giac::matrice M(n);
  c_complexptr2matrice(x,n,n,M);
  giac::gen g=giac::mdet(M,giac::context0);
  g=giac::evalf_double(g,1,giac::context0);
  return gen2c_complex(g);
}

void c_sprint_double(char * s,double d){
  sprint_double(s,d);
}

extern "C" void raisememerr();

void c_turtle_forward(double d){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  //const context * contextptr=caseval_context();
  giac::_avance(d,cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_left(double d){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  giac::_tourne_gauche(d,cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_up(int i){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  if (i)
    giac::_leve_crayon(0,cascontextptr);
  else
    giac::_baisse_crayon(0,cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_goto(double x,double y){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  giac::_position(makesequence(x,y),cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_cap(double x){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  giac::_cap(x,cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_crayon(int i){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  giac::_crayon(i,cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_rond(int x,int y,int z){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  giac::_rond(makesequence(x,y,z),cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_disque(int x,int y,int z,int centre){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  if (centre)
    giac::_disque_centre(makesequence(x,y,z),cascontextptr);
  else
    giac::_disque(makesequence(x,y,z),cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_fill(int i){
  gen arg(vecteur(0));
  if (i==0) 
    arg.subtype=_SEQ__VECT;
  context * cascontextptr=(context *)caseval("caseval contextptr");
  giac::_polygone_rempli(arg,cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_fillcolor(double r,double g,double b,int entier){
  context * cascontextptr=(context *)caseval("caseval contextptr");
  if (entier)
    giac::_polygone_rempli(makesequence(int(r),int(g),int(b)),cascontextptr);
  else
    giac::_polygone_rempli(makesequence(r,g,b),cascontextptr);
  py_ck_ctrl_c();
}

void c_turtle_getposition(double * x,double * y){
  giac::context * cascontextptr=(giac::context *) giac::caseval("caseval contextptr");
  giac::gen arg(vecteur(0)); arg.subtype=_SEQ__VECT;
  giac::gen g=giac::_position(arg,cascontextptr);
  if (g.type==_VECT && g._VECTptr->size()==2){
    gen a=g._VECTptr->front(),b=g._VECTptr->back();
    a=evalf_double(a,1,cascontextptr);
    b=evalf_double(b,1,cascontextptr);
    *x=a._DOUBLE_val;
    *y=b._DOUBLE_val;
  }
}

void c_turtle_towards(double x,double y){
  double x0,y0;
  c_turtle_getposition(&x0,&y0);
  double t=std::atan2(x-x0,y-y0);
  c_turtle_cap(t*180/M_PI);
}

void c_turtle_color(int c){
  giac::context * cascontextptr=(giac::context *) giac::caseval("caseval contextptr");
  giac::_crayon(c,cascontextptr);
}

int c_turtle_getcolor(){
#ifdef TURTLETAB
  if (giac::turtle_stack_size==0)
    return -1;
  return giac::tablogo[giac::turtle_stack_size-1].color;
#else
  int l=giac::turtle_stack().size();
  if (l==0)
    return -1;
  return giac::turtle_stack()[l-1].color;
#endif
}

int c_turtle_getcap(){
#ifdef TURTLETAB
  if (giac::turtle_stack_size==0)
    return -1;
  return giac::tablogo[giac::turtle_stack_size-1].theta;
#else
  int l=giac::turtle_stack().size();
  if (l==0)
    return -1;
  return giac::turtle_stack()[l-1].theta;
#endif
}

void c_turtle_clear(int clrpos){
  giac::context * cascontextptr=(giac::context *) giac::caseval("caseval contextptr");
  giac::_efface(giac::vecteur(0),cascontextptr);
}

void c_turtle_show(int visible){
  giac::context * cascontextptr=(giac::context *) giac::caseval("caseval contextptr");
  if (visible)
    giac::_montre_tortue(0,cascontextptr);
  else
    giac::_cache_tortue(0,cascontextptr);
}

extern "C" void sync_screen(){ Bdisp_PutDisp_DD(); }

int kbd_filter(int key){
  if (key==KEY_CTRL_LEFT) return 0;
  if (key==KEY_CTRL_RIGHT) return 3;
  if (key==KEY_CTRL_UP) return 1;
  if (key==KEY_CTRL_DOWN) return 2;
  if (key==KEY_CTRL_EXE) return 4;
  if (key==KEY_CTRL_EXIT) return 5;
  return key;
}  

void c_turtle_fillcolor1(int c){} // FIXME fake function
#endif // MICROPY_LIB

#ifndef MICROPY_LIB
extern "C" void sync_screen(){ Bdisp_PutDisp_DD(); }
#endif

// string translations
#ifdef NUMWORKS
#include "numworks_translate.h"
#else
#include "aspen_translate.h"
#endif
bool tri2(const char4 & a,const char4 & b){
  int res= strcmp(a[0],b[0]);
  return res<0;
}

int giac2aspen(int lang){
  switch (lang){
  case 0: case 2:
    return 1;
  case 1:
    return 3;
  case 3:
    return 5;
  case 6:
    return 7;
  case 8:
    return 2;
  case 5:
    return 4;
  }
  return 0;
}

const char * gettext(const char * s) {
#if 1
  return s;
#else
  // 0 and 2 english 1 french 3 sp 4 el 5 de 6 it 7 tr 8 zh 9 pt
  int aspenlang=giac2aspen(lang);
  char4 s4={s};
  std::pair<char4 * const,char4 *const> pp=equal_range(aspen_giac_translations,aspen_giac_translations+aspen_giac_records,s4,tri2);
  if (pp.first!=pp.second && 
      pp.second!=aspen_giac_translations+aspen_giac_records &&
      (*pp.first)[aspenlang]){
    return (*pp.first)[aspenlang];
  }
  return s;
#endif
}

  void process_freeze(){
    if (freezeturtle){
      displaylogo();
      freezeturtle=false;
      return;
    }
    if (giac::freeze){
      giac::freeze=false;
      for (;;){
#ifdef NSPIRE_NEWLIB
	DefineStatusMessage((char*)((lang==1)?"Ecran fige. Taper esc":"Screen frozen. Press esc."), 1, 0, 0);
#else
	DefineStatusMessage((char*)((lang==1)?"Ecran fige. Taper EXIT":"Screen frozen. Press EXIT."), 1, 0, 0);
#endif
	DisplayStatusArea();
	int key;
	ck_getkey(&key);
	if (key==KEY_CTRL_EXIT || key==KEY_CTRL_AC)
	  break;
      }
    }
  }    

// int Keyboard_SpyMatrixCode(char* column, char* row) ;
// returns the current number of key codes in the key buffer.
bool shiftstate=false,alphastate=false,alphalock=false,alphamaj=true,oldshiftstate=false,oldalphastate=false;
short int timeout_delay=300;

void casiostatus(const char * menu,const char * shiftmenu,const char * alphamenu,int color){
  static const char * prev=0;
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
  bool setstatus=!is_emulator;
  if ((unsigned)menu==1){
    menu=0;
    setstatus=false;
  }
  if (!menu)
    prev=menu;
  if (setstatus){
    int casioset=0;
    if (shiftstate)
      casioset=(1);
    else if (alphastate){
      if (alphalock){
	if (alphamaj)
	  casioset=(0x84);
	else
	  casioset=(0x88);
      }
      else {
	if (alphamaj)
	  casioset=(0x4);
	else
	  casioset=(0x8);
      }
    } 
    SetSetupSetting(0x14,casioset);
  }
  if (menu && shiftmenu && alphamenu){
    int px=0*3,py=58*3;
    const char * cur=0;
    if (shiftstate)
      cur=shiftmenu;
    else {
      if (alphastate && !alphalock)
	cur=alphamenu;
      else
	cur=menu;
    }
    if (cur!=prev){
      prev=cur;
      PrintMini(&px,&py,(unsigned char *)cur,0,0xFFFFFFFF,0,0,COLOR_BLACK,color, 1, 0);
    }
  }
}

const unsigned short translated_keys[] =
{ 
  // non shifted
  KEY_CTRL_F1,KEY_CTRL_F2,KEY_CTRL_F3,KEY_CTRL_F4,KEY_CTRL_F5,KEY_CTRL_F6,
  KEY_CTRL_SHIFT,KEY_CTRL_OPTN,KEY_CTRL_VARS,KEY_CTRL_MENU,KEY_CTRL_LEFT,KEY_CTRL_UP,
  KEY_CTRL_ALPHA,KEY_CHAR_SQUARE,KEY_CHAR_POW,KEY_CTRL_EXIT,KEY_CTRL_DOWN,KEY_CTRL_RIGHT,
  KEY_CTRL_XTT,KEY_CHAR_LOG,KEY_CHAR_LN,KEY_CHAR_SIN,KEY_CHAR_COS,KEY_CHAR_TAN,
  KEY_CHAR_FRAC,KEY_CTRL_SD,KEY_CHAR_LPAR,KEY_CHAR_RPAR,KEY_CHAR_COMMA,KEY_CHAR_STORE,
  '7','8','9',KEY_CTRL_DEL,KEY_CTRL_AC,KEY_CTRL_AC,
  '4','5','6',KEY_CHAR_MULT,KEY_CHAR_DIV,65535,
  '1','2','3',KEY_CHAR_PLUS,KEY_CHAR_MINUS,65535,
  '0','.',KEY_CHAR_EXP,KEY_CHAR_PMINUS,KEY_CTRL_EXE,65535,
  // shifted
  KEY_CTRL_F7,KEY_CTRL_F8,KEY_CTRL_F9,KEY_CTRL_F10,KEY_CTRL_F11,KEY_CTRL_F12,
  KEY_CTRL_SHIFT,KEY_SHIFT_OPTN,KEY_CTRL_PRGM,KEY_CTRL_SETUP,KEY_SHIFT_LEFT,KEY_CTRL_PAGEUP,
  KEY_CTRL_ALPHA,KEY_CHAR_ROOT,KEY_CHAR_POWROOT,KEY_CTRL_QUIT,KEY_CTRL_PAGEDOWN,KEY_SHIFT_RIGHT,
  KEY_CHAR_ANGLE,KEY_CHAR_EXPN10,KEY_CHAR_EXPN,KEY_CHAR_ASIN,KEY_CHAR_ACOS,KEY_CHAR_ATAN,
  KEY_CTRL_MIXEDFRAC,KEY_CTRL_FRACCNVRT,KEY_CHAR_CUBEROOT,KEY_CHAR_RECIP,KEY_LOAD,KEY_SAVE,
  KEY_CTRL_CAPTURE,KEY_CTRL_CLIP,KEY_CTRL_PASTE,KEY_CTRL_INS,KEY_PRGM_ACON,65535,
  KEY_CTRL_CATALOG,KEY_CTRL_FORMAT,KEY_CTRL_F16,KEY_CHAR_LBRACE,KEY_CHAR_RBRACE,65535,
  KEY_CHAR_LIST,KEY_CHAR_MAT,KEY_CTRL_F13,KEY_CHAR_LBRCKT,KEY_CHAR_RBRCKT,65535,
  KEY_CHAR_IMGNRY,KEY_CHAR_EQUAL,KEY_CHAR_PI,KEY_CHAR_ANS,KEY_CHAR_CR,65535,
  // alpha
  KEY_CTRL_F1,KEY_CTRL_F2,KEY_CTRL_F3,KEY_CTRL_F4,KEY_CTRL_F5,KEY_CTRL_F6,
  KEY_CTRL_SHIFT,KEY_CTRL_OPTN,KEY_CTRL_VARS,9,KEY_CTRL_LEFT,KEY_CTRL_UP,
  KEY_CTRL_ALPHA,KEY_CHAR_VALR,KEY_CHAR_THETA,KEY_CTRL_EXIT,KEY_CTRL_DOWN,KEY_CTRL_RIGHT,
  'A','B','C','D','E','F',
  'G','H','I','J','K','L',
  'M','N','O',KEY_CTRL_UNDO,KEY_CTRL_DEL,KEY_CTRL_AC,
  'P','Q','R','S','T',65535,
  'U','V','W','X','Y',65535,
  'Z',' ','"',KEY_CHAR_ANS,KEY_CTRL_EXE,65535,
  // alpha shifted
  KEY_CTRL_F1,KEY_CTRL_F2,KEY_CTRL_F3,KEY_CTRL_F4,KEY_CTRL_F5,KEY_CTRL_F6,
  KEY_CTRL_SHIFT,KEY_CTRL_OPTN,KEY_CTRL_VARS,9,KEY_CTRL_LEFT,KEY_CTRL_UP,
  KEY_CTRL_ALPHA,KEY_CHAR_VALR,KEY_CHAR_THETA,KEY_CTRL_EXIT,KEY_CTRL_DOWN,KEY_CTRL_RIGHT,
  'a','b','c','d','e','f',
  'g','h','i','j','k','l',
  'm','n','o',KEY_CTRL_UNDO,KEY_CTRL_DEL,KEY_CTRL_AC,
  'p','q','r','s','t',65535,
  'u','v','w','x','y',65535,
  'z',' ','"',KEY_CHAR_ANS,KEY_CTRL_EXE,65535,
};

static volatile int menutimer=0;
void clear_menu_timer(){
  if (menutimer > 0) {
    Timer_Stop(menutimer);
    Timer_Deinstall(menutimer);
    menutimer=0;
  }
}

void send_menu_key(){
  if (menutimer > 0) {
    clear_menu_timer();
    // OS_InnerWait_ms(50);
    // menu row 9 col 4
    // Keyboard_PutKeycode(4,9,KEY_CTRL_MENU);
    Keyboard_PutKeycode(4,9,0);
  }
}

void set_menu_timer(){
  if (menutimer) return;
  menutimer = Timer_Install(0,send_menu_key, 50);
  if (menutimer > 0) {
    Timer_Start(menutimer);
  }
}

void MENU_save(){
  console_changed=0;
}  

int in_ckgetkey(int * keyptr,int waitforkey,const char * menu,const char * shiftmenu,const char * alphamenu,int menucolorbg){
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
  int keyflag=GetSetupSetting( (unsigned int)0x14);
  if (is_emulator){
    alphalock= keyflag &0x80;
    if ( (keyflag&4) | (keyflag &8))
      alphastate=oldalphastate=true;
    else
      alphastate=oldalphastate=false;
    if (keyflag & 1)
      shiftstate=oldshiftstate=true;
    else
      shiftstate=oldshiftstate=false;
    casiostatus(0,0,0,0);
    casiostatus(menu,shiftmenu,alphamenu,menucolorbg);
    GetKey(keyptr);
    if (*keyptr>=KEY_CTRL_F1 && *keyptr<=KEY_CTRL_F6 && (keyflag & 1))
      *keyptr += 9906; // shift-F1 -> F7
    if (*keyptr>=KEY_CTRL_F1 && *keyptr<=KEY_CTRL_F6 && (alphastate && !alphalock))
      *keyptr += 9912; // ALPHA-F1 -> F13
    return 1;
  }
  int col, row;
  unsigned short keycode=0;
  shiftstate=false;alphastate=false;alphalock=false;alphamaj=true;
  // Keyboard input mode. 0 for standard, 0x01 for Shift, 0x04 for Alpha, 0x08 for lower-case Alpha, 0x84 for Alpha-lock, 0x88 for lower-case Alpha-Lock. 
  if ( (keyflag&4) | (keyflag &8))
    alphastate=true;
  if (keyflag&0x80)
    alphalock=true;
  if (keyflag==0x88)
    alphamaj=false;
  if (keyflag & 1)
    shiftstate=true;
  while (1){
    if (waitforkey){
      casiostatus(menu,shiftmenu,alphamenu,menucolorbg);
      //DefineStatusMessage(shiftstate?shiftmenu:menu,1,0,0);
      DisplayStatusArea();
      Bdisp_PutDisp_DD ();
    }
    SetSetupSetting(0x14,0); // disable OFF
    int ret=GetKeyWait_OS(&col,&row, waitforkey?2:1, timeout_delay /*timeout_period*/, 1 /* 0: handle menu key*/, &keycode) ;
    // ret==0 : no key available, ret==1 key available, ret==2 timeout
    if (ret==0)
      return -1;
    if (!shiftstate && (col==4 && row==9)){
      MENU_save();
      set_menu_timer();
      GetKey(keyptr);
      //*keyptr=KEY_SHUTDOWN;
      return 1;      
    }
    if (ret==2 /* timeout */ ){
      MENU_save();
      //printf("timeout shutdown \n");
#if 1
      set_menu_timer();
      GetKey(keyptr);
      //*keyptr=KEY_SHUTDOWN;
      return 1;      
#endif
#if 1
      if (lang)
	print_msg12("Pour relancer KhiCAS, tapez les touches","MENU tan MENU");
      else
	print_msg12("To restart KhiCAS, type keys","MENU tan MENU");
      Bdisp_PutDisp_DD ();
      set_menu_timer();
      GetKey(keyptr);
      // save_session();
      // SetQuitHandler(0);
      // PowerOff(1);
      for (;;){
	GetKey(keyptr);
      }
#endif
      print_msg12("Auto shutdown","MENU");	
      Bdisp_PutDisp_DD ();
      set_menu_timer();
      GetKey(keyptr);
      *keyptr=KEY_SHUTDOWN;
      return 1;
      // continue;
    }
    if (0 && keycode>0){
      *keyptr=keycode;
      return ret;
    }
    int code;
    if (col==1)
      code=35;
    else
      code=(10-row)*6+(7-col);
#if 0
    char buf[256],buf2[256];
    sprint_int(buf,row);
    sprint_int(buf2,col);
    *logptr(contextptr) << "row " << buf << " col " << buf2 << '\n';
#endif
    if (code==6){ // shift
      shiftstate=!shiftstate;
      continue;
      *keyptr=KEY_CTRL_SHIFT; return 1; // does not work
    }
    if (code==12){ // alpha
      if (shiftstate){
	alphastate=true;
	alphalock=true;
	shiftstate=false;
      }
      else {
	if (alphastate){
	  alphastate=false;
	  alphalock=false;
	}
	else
	  alphastate=true;
      }
      continue;
      *keyptr=KEY_CTRL_ALPHA; return 1;// does not work
    }
    if (code<0 || code>=54/* USB connect */){
      if (lang)
	print_msg12("Taper sur une touche SVP","");
      else
	print_msg12("Please type any key","");
      Bdisp_PutDisp_DD ();
      GetKey(keyptr);
      return 1;
    }
    if (shiftstate && code==35){
      shiftstate=false;
      // print_msg12("Type MENU SHIFT ON to shutdown","Taper MENU SHIFT ON pour eteindre");
#if 1
      if (lang)
	print_msg12("Retour menu home. Pour eteindre","tapez a nouveau SHIFT AC/ON");
      else
	print_msg12("Back to home menu. To shutdown","type again SHIFT AC/ON");	
      Bdisp_PutDisp_DD ();
      OS_InnerWait_ms(1000);
      set_menu_timer();
      GetKey(keyptr);
      //*keyptr=KEY_SHUTDOWN;
      return 1;      
#endif
      if (lang)
	print_msg12("Pour eteindre, tapez les touches","MENU SHIFT AC/ON");
      else
	print_msg12("To shutdown, type the keys","SHIFT AC/ON");	
      continue;
      Bdisp_PutDisp_DD ();
      OS_InnerWait_ms(1000);
      //set_menu_timer();
      //GetKey(keyptr);
      casiostatus(menu,shiftmenu,alphamenu,menucolorbg);
      *keyptr=KEY_SHUTDOWN;
      return 1;
    }
    // col and row translation to keycode
    if (alphastate){
      if (alphamaj)
	code += 2*9*6;
      else
	code += 3*9*6;
    }
    else 
      if (shiftstate)
	code += 9*6;
    *keyptr=translated_keys[code];
    if (alphalock && *keyptr==KEY_CTRL_UNDO)
      *keyptr=KEY_CTRL_DEL;
    if (alphastate && !alphalock){
      if (*keyptr==KEY_CTRL_EXIT)
	*keyptr=65535;
      if (*keyptr>=KEY_CTRL_F1 && *keyptr<=KEY_CTRL_F6)
	*keyptr += KEY_CTRL_F13-KEY_CTRL_F1;
    }
#if 0
    char buf[512];
    sprintf(buf,"code %i key %i ",code,*keyptr);
    Console_Output(buf);
#endif
    oldshiftstate=shiftstate;
    shiftstate=false;
    oldalphastate=alphastate;
    if (!alphalock)
      alphastate=false;
    casiostatus(menu,shiftmenu,alphamenu,menucolorbg);
    return 1;
  }
}

int ck_getkey(int * keyptr){
  if (xcas_python_eval==1)
    python_compat(4,contextptr);
  return in_ckgetkey(keyptr,1,0,0,0,12345); /* wait for key */
}

// check integrity of RAM and reload it if required
unsigned ram2Msize=0;

// reload=0 check, 1=check and reload, 2=force reload
int chk_ram2M(const char * filename,int reload){
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
  //bool is_emulator = !memcmp((void *)0xa001ffd0, "\xff\xff\xff\xff\xff\xff\xff\xff", 8);
  // emulator address 0x88200000, calculator address 0xac200000
  unsigned ram2Maddr, ram2Mendaddr;
  if (is_emulator)
    ram2Maddr=0x88200000;
  else 
    ram2Maddr=0xac200000;
  ram2Mendaddr=ram2Maddr+ram2Msize;
  unsigned * ram2M=(unsigned *) ram2Maddr,*ram2Mend=(unsigned *) ram2Mendaddr;
  const unsigned chkinit=0x12345678;
  unsigned chk=chkinit,*ptr;
  if (ram2Msize && reload<2){
    for (ptr=ram2M;ptr<ram2Mend;ptr+=4){
      chk ^= (ptr[0] ^ ptr[1] ^ptr[2] ^ ptr[3]);
    }
    if (chk==*ptr)
      return 1; // proba to return true with corruption is tiny, about 1/4e9
    if (!reload)
      return 0;
  }
  // load ram part, filename="khicas.882" or "khicas.ac2"
  int nchar=strlen(filename);
  if (nchar<5 || filename[nchar-4]!='.' || filename[nchar-1]!='2')
    return -1;
  unsigned short pFile[nchar+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1);
  if (is_emulator){
    pFile[nchar-3]='8';
    pFile[nchar-2]='8';
  }
  else {
    pFile[nchar-3]='a';
    pFile[nchar-2]='c';
  }
  int hf = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if (hf < 0)
    return -2; // nothing to load
  int size=Bfile_GetFileSize_OS(hf);
  if (size>3*1024*1024-4
      || Bfile_ReadFile_OS(hf,ram2M,size,0)!=size
      ){
    Bfile_CloseFile_OS(hf);
    return -3;
  }
  ram2Msize=4*((size+3)/4);
  ram2Mendaddr=ram2Maddr+ram2Msize;
  ram2Mend=(unsigned *) ram2Mendaddr;
  Bfile_CloseFile_OS(hf);
  // wait a little?
  // wait_1ms(100);
  // compute chksum
  chk=chkinit;
  for (ptr=ram2M;ptr<ram2Mend;ptr+=4){
    chk ^= (ptr[0] ^ ptr[1] ^ptr[2] ^ ptr[3]);
  }
  *ptr=chk; // save chksum
  return 1;
}

void do_restart(){
  giac::_restart(gen(vecteur(0),_SEQ__VECT),contextptr);
}

void chk_restart(){
  drawRectangle(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24, COLOR_WHITE);
  if (confirm(chk_restart_string1,chk_restart_string2)==KEY_CTRL_F6)
    do_restart();
}



//const char * keywords[]={"do","faire","for","if","return","while"}; // added to lexer_tab_int.h

int find_color(const char * s){
  if (s[0]=='"')
    return 4;
  if (!isalpha(s[0]))
    return 0;
  char buf[256];
  const char * ptr=s;
  for (int i=0;i<255 && (isalphanum(*ptr) || *ptr=='_'); ++i){
    ++ptr;
  }
  strncpy(buf,s,ptr-s);
  buf[ptr-s]=0;
  if (xcas_python_eval>=0 && (strcmp(buf,"def")==0 || strcmp(buf,"import")==0))
    return 1;
#ifdef MICROPY_LIB
  if (is_python_builtin(buf))
    return 3;
#endif
  //int pos=dichotomic_search(keywords,sizeof(keywords),buf);
  //if (pos>=0) return 1;
  gen g;
  int token=find_or_make_symbol(buf,g,0,false,contextptr);
#ifdef MICROPY_LIB
  if (xcas_python_eval==1){
    if (micropy_ck_eval("")!=RAND_MAX){
      int tok=mp_token(buf);
      if (tok) return tok;
      if (token==T_NUMBER)
	return 2;
      if (token==T_UNARY_OP || token==T_UNARY_OP_38 || token==T_LOGO)
	return 0;
      if (token!=T_SYMBOL)
	return 1;
      return 0;
    }
  }
#endif
  //*logptr(contextptr) << s << " " << buf << " " << token << " " << g << endl;
  if (token==T_UNARY_OP || token==T_UNARY_OP_38 || token==T_LOGO)
    return 3;
  if (token==T_NUMBER)
    return 2;
  if (token!=T_SYMBOL)
    return 1;
  return 0;
}

#if 0
const char * unary_function_ptr_name(void * ptr){
  const giac::unary_function_ptr * u=(const unary_function_ptr *) ptr;
  return u->ptr()->s;
}
#endif

void stop(const char * s){
}

extern "C" int kbd_convert(int r,int c);
int kbd_convert(int r,int c){
  if (r==1 && c==1)
    return KEY_CTRL_AC;
  if (r==2){
    if (c==7) return KEY_CHAR_0;
    if (c==6) return KEY_CHAR_DP;
    if (c==5) return KEY_CHAR_EXPN10;
    if (c==4) return KEY_CHAR_PMINUS;
    if (c==3) return 4; // KEY_CTRL_EXE;
  }
  if (r==3){
    if (c==7) return KEY_CHAR_1;
    if (c==6) return KEY_CHAR_2;
    if (c==5) return KEY_CHAR_3;
    if (c==4) return KEY_CHAR_PLUS;
    if (c==3) return KEY_CHAR_MINUS;
  }
  if (r==4){
    if (c==7) return KEY_CHAR_4;
    if (c==6) return KEY_CHAR_5;
    if (c==5) return KEY_CHAR_6;
    if (c==4) return KEY_CHAR_MULT;
    if (c==3) return KEY_CHAR_DIV;
  }
  if (r==5){
    if (c==7) return KEY_CHAR_FRAC;
    if (c==6) return KEY_CHAR_H;
    if (c==5) return KEY_CHAR_LPAR;
    if (c==4) return KEY_CHAR_RPAR;
    if (c==3) return KEY_CHAR_COMMA;
    if (c==2) return KEY_CHAR_STORE;
  }
  if (r==6){
    if (c==7) return KEY_CTRL_XTT;
    if (c==6) return KEY_CHAR_LOG;
    if (c==5) return KEY_CHAR_LN;
    if (c==4) return KEY_CHAR_SIN;
    if (c==3) return KEY_CHAR_COS;
    if (c==2) return KEY_CHAR_TAN;
  }
  if (r==8){
    if (c==7) return KEY_CTRL_ALPHA;
    if (c==6) return KEY_CHAR_SQUARE;
    if (c==5) return KEY_CHAR_POW;
    if (c==4) return 5; // KEY_CTRL_EXIT;
    if (c==3) return 2; // KEY_CTRL_DOWN;
    if (c==2) return 3; // KEY_CTRL_RIGHT;
  }
  if (r==9){
    if (c==7) return KEY_CTRL_SHIFT;
    if (c==6) return KEY_CTRL_OPTN;
    if (c==5) return KEY_CTRL_VARS;
    if (c==4) return KEY_CTRL_MENU;
    if (c==3) return 0; // KEY_CTRL_LEFT;
    if (c==2) return 1; // KEY_CTRL_UP;
  }
  if (r==10){
    if (c==7) return KEY_CTRL_F1;
    if (c==6) return KEY_CTRL_F2;
    if (c==5) return KEY_CTRL_F3;
    if (c==4) return KEY_CTRL_F4;
    if (c==3) return KEY_CTRL_F5;
    if (c==2) return KEY_CTRL_F6;
  }
  return 0;
}

static int FindZeroedMemory(void *start)
{
  /* Look for zero-longwords every 16 bytes */
  int size = 0;
  /* Limit to 6 MB since the fx-CG 50 doesn't have any more memory and
     anything after the RAM is likely to be zero non-writable */
  while(size < 6*1024*1024 && !*(uint32_t *)(start + size))
    size += 16;
  
  /* Round down to a multiple of 4096 */
  return size & ~0xfff;
}

#ifdef GINT_MALLOC
#define VAR_HEAP
#ifdef VAR_HEAP
__attribute__((aligned(4))) char malloc_heap[
					     240*1024
					     //216*1024
					     ];
#else
/* Unused part of user stack; provided by linker script */
extern char sextra, eextra;
#endif

kmalloc_arena_t static_ram = { 0 },ram3M={0};
int get_free_memory(){
  kmalloc_gint_stats_t * s;
  if (pythonjs_static_heap)
    s=kmalloc_get_gint_stats(&ram3M);
  else
    s=kmalloc_get_gint_stats(&static_ram);
  if (!s) return 0;
  int res=s->free_memory;
  return res;
}
#else
int get_free_memory(){
  return freeslotmem();
}
#endif

#define GetOSVersion() ((char *)0x80020020)

static void *my_bf_realloc(void *opaque, void *ptr, size_t size)
{
    return realloc(ptr, size);
}

int calculator=0; // -1 means OS not checked, 0 unknown, 1 cg50 or 90, 2 emu 50 or 90, 3 other
int cpu_speed=0; // initial speed
int inexammode=0;
#define pagesize (128*1024)

void quit_handler(){
  console_changed=0;
}

int main1(){
  unsigned exammodestart=0xa0b20000,exammodestep=0x40/sizeof(unsigned);
  unsigned * exammodeptr=(unsigned *) exammodestart;
  for (;(unsigned) exammodeptr<exammodestart+pagesize;exammodeptr+=exammodestep){
    if (*exammodeptr==0xffffffff)
      break;
  }
  if ((unsigned)exammodeptr>exammodestart){
    unsigned char ch=*(unsigned char *) ( ((unsigned) exammodeptr)-0x40+3 );
    inexammode = (ch & 0xf0);
  }
  if (inexammode && (*(char*)0x80000305)!=0x55){
#ifdef MPM
    print_msg12("Exam mode","");
#else
    print_msg12("Exam mode","Press MENU key");
#endif
    int key;
    GetKey(&key);
#ifndef MPM
    for(;;)
      GetKey(&key);
#endif
    return 0;
  }
#if 1
  /* Allow the user to use memory past the 2 MB line on tested OS versions */
  // int availram=FindZeroedMemory((void *)0xac200000);
  char const *osv = GetOSVersion();
  // { print_msg12("OS version",osv); int key; ck_getkey(&key); }
  if (!strncmp(osv, "03.", 3) && osv[3] <= '8'){ // 3.60 or earlier
    char buf[256];
    // sprintf(buf,"%i",availram);
    strncpy(buf,osv,8);
    buf[8]=0;
    // { print_msg12("OS ok version",buf); int key; ck_getkey(&key); }
  }
  else {
    char buf1[10],buf[256],buf2[16]={0};
    // sprintf(buf,"%i",availram);
    strncpy(buf1,osv,8);
    buf1[8]=0;
    sprintf(buf,"OS %s not checked",buf1);
    calculator=-1;
    int key;
    print_msg12(buf,"F6: continue anyway");
    GetKey(&key);
    if (key!=KEY_CTRL_F6){
      print_msg12("Press MENU to leave","");
      int key;
      for(;;)
        GetKey(&key);
      return 0;
    }
  }
#endif
  // confirm("main","0");
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
  uint32_t stack;
  __asm__("mov r15, %0" : "=r"(stack));
  bool prizmoremu=stack<0x8c000000;
  if (calculator==0)
    calculator=prizmoremu?2:1;
  // confirm("main","1");
#ifdef GINT_MALLOC
  /* À appeler une seule fois au début de l'exécution */
  kmalloc_init();
#if defined MICROPY_LIB && defined VAR_HEAP // area reserved for micropython heap
  pythonjs_heap_size=sizeof(malloc_heap);
  pythonjs_static_heap=malloc_heap;
#else
  /* Ajouter une arène sur la RAM inutilisée */
  static_ram.name = "_uram";
  static_ram.is_default = 0; // 0 disable this area, 1 enable
#ifdef VAR_HEAP
  static_ram.start = malloc_heap;
  static_ram.end = malloc_heap+sizeof(malloc_heap);
#else
  //void *malloc_start = &sextra;
  //void *malloc_end = &eextra;
  //int malloc_size = malloc_end - malloc_start;
  static_ram.start = &sextra;
  static_ram.end = &eextra;
#endif
  kmalloc_init_arena(&static_ram, true);
  kmalloc_add_arena(&static_ram);
#endif // ndef micropylib
  if (prizmoremu) {
    /* Prizm ou émulateur Graph 90+E */
    ram3M.name="_3M";
    ram3M.is_default=1;
    ram3M.start=0x88480000;
    ram3M.end=ram3M.start+0x180000;
    kmalloc_init_arena(&ram3M, true);
    kmalloc_add_arena(&ram3M);
  }
  else {
    /* Graph 90+E & FXCG50(?) */
    ram3M.name="_3M";
    ram3M.is_default=1;
    ram3M.start=0x8c480000;
    ram3M.end=ram3M.start+0x180000;
    kmalloc_init_arena(&ram3M, true);
    kmalloc_add_arena(&ram3M);
  }
#endif // GINT_MALLOC
  int errcode=chk_ram2M(ram_filename,2); // 2=force reload
  if (errcode<=0){
    char buf[]="khicas90.ac2 err=0";
    buf[6]=ram_filename[13]; // \fls0\emucas90
    if (buf[6]=='0')
      buf[6]=ram_filename[12]; // \fls0\excas90
    if (is_emulator){
      buf[9]=buf[10]='8';
    }
    buf[strlen(buf)-1]='0'-errcode;
    print_msg12("Fatal: unable to load ram part",buf);
    int key;
    for(;;)
      GetKey(&key);
    return 0;
  }
  errcode=chk_ram2M(ram_filename,false);
  if (errcode<=0){
    char buf[]="khicas.ac2 err=0";
    if (is_emulator){
      buf[7]=buf[8]='8';
    }
    buf[strlen(buf)-1]='0'-errcode;
    print_msg12("Fatal: unable to check ram part",buf);
    int key;
    for(;;)
      GetKey(&key);
    return 0;
  }
  // confirm("main","1.5");
  cpu_speed=0;
  // Overclock/session QR startup is out of A-level scope.
  //{ char buf[2]={0,0}; buf[0]='0'+cpu_speed; print_msg12("CPU speed (0 unknown, 1-5)",buf); int key; GetKey(&key); }
#ifdef BF2GMP
  // initialize context
  bf_ctx_ptr=(bf_context_t *) malloc(sizeof(bf_context_t));
  bf_context_init(bf_ctx_ptr, my_bf_realloc, NULL);
  
#endif
  //confirm("main","2");
  context ct;
  contextptr=&ct;
  SetQuitHandler(quit_handler);
  turtle();
#ifdef TURTLETAB
  turtle_stack_size=0;
#else
  turtle_stack(); // required to init turtle
#endif
  int key;
  Console_Init();
  // return 1;
  Bdisp_AllClr_VRAM();
  Bdisp_EnableColor(1);
  EnableStatusArea(0);
  DefineStatusAreaFlags(3, SAF_BATTERY | SAF_TEXT | SAF_GLYPH | SAF_ALPHA_SHIFT, 0, 0);
  rand_seed(RTC_GetTicks(),contextptr);
  VRAM_base = (color_t*)GetVRAMAddress();
  // confirm("main","2.5");
  // File-backed session reset/load is disabled for CasioCAS A-level mode.
  restore_session("session");
  //ck_getkey(&key);
#if 0
  //int tableau=0;
  //int availram=((unsigned) &tableau) & 0xffffff;
  //tableau += availram;
  int availram=FindZeroedMemory((void *)0xac200000);
  Console_Output(print_INT_(availram).c_str());
  Console_NewLine(LINE_TYPE_OUTPUT,1);
#endif
  //gen dbgg=123; const char * dbgptr=dbgg.dbgprint();  confirm(dbgptr,"");
  //micropy_ck_eval("");
  // confirm("main","3");
  Console_Disp();
  //ck_getkey(&key);
  // disable Catalog function throughout the add-in, as we don't know how to make use of it:
  Bkey_SetAllFlags(0x80);
  unsigned char *expr=0;
  while(1){
    if((expr=Console_GetLine())==NULL) stop("memory error");
    if (strcmp((const char *)expr,"restart")==0){
      if (confirm(main_string1,main_string2)!=KEY_CTRL_F6){
	Console_Output((const unsigned char *)" cancelled");
	Console_NewLine(LINE_TYPE_OUTPUT,1);
	//ck_getkey(&key);
	Console_Disp();
	continue;
      }
    }
    // should save in another file
    if (strcmp((const char *)expr,"=>")==0 || strcmp((const char *)expr,"=>\n")==0){
      console_changed=0;
      Console_Output((unsigned char*)"Disabled");
    }
    else 
      run((char *)expr);
    //print_mem_info();
    Console_NewLine(LINE_TYPE_OUTPUT,1);
    //ck_getkey(&key);
    Console_Disp();
  }
  for(;;)
    ck_getkey(&key);
  return 1; 
}

int main(){
#ifdef MICROPY_LIB
  mp_stack_ctrl_init();
  giac::micropy_ptr=micropy_ck_eval;
#endif
  main1();
}

bool islogo(const gen & g){
  if (g.type!=_VECT || g._VECTptr->empty()) return false;
  if (g.subtype==_LOGO__VECT) return true;
  const vecteur & v=*g._VECTptr;
  if (islogo(v.back()))
    return true;
  for (size_t i=0;i<v.size();++i){
    if (v[i].type==_VECT && v[i].subtype==_LOGO__VECT)
      return true;
  }
  return false;
}

bool ispnt(const gen & g){
  if (g.is_symb_of_sommet(giac::at_pnt))
    return true;
  if (g.type!=_VECT || g._VECTptr->empty())
    return false;
  return ispnt(g._VECTptr->back());
}

giac::gen eqw(const giac::gen & ge,bool editable){
  bool edited=false;
#ifdef CURSOR
  Cursor_SetFlashOff();
#endif
  giac::gen geq(_copy(ge,contextptr));
  // if (ge.type!=giac::_DOUBLE_ && giac::has_evalf(ge,geq,1,contextptr)) geq=giac::symb_equal(ge,geq);
  int line=-1,col=-1,nlines=0,ncols=0,listormat=0;
  xcas::Equation eq(0,0,geq);
  giac::eqwdata eqdata=xcas::Equation_total_size(eq.data);
  if (eqdata.dx>1.5*LCD_WIDTH_PX || eqdata.dy>.7*LCD_HEIGHT_PX){
    if (eqdata.dx>2.25*LCD_WIDTH_PX || eqdata.dy>2*LCD_HEIGHT_PX)
      eq.attr=giac::attributs(14,COLOR_WHITE,COLOR_BLACK);
    else
      eq.attr=giac::attributs(16,COLOR_WHITE,COLOR_BLACK);
    eq.data=0; // clear memory
    eq.data=xcas::Equation_compute_size(geq,eq.attr,LCD_WIDTH_PX,contextptr);
    eqdata=xcas::Equation_total_size(eq.data);
  }
  int dx=(eqdata.dx-LCD_WIDTH_PX)/2,dy=LCD_HEIGHT_PX-2*STATUS_AREA_PX+eqdata.y;
  if (geq.type==_VECT){
    nlines=geq._VECTptr->size();
    if (eqdata.dx>=LCD_WIDTH_PX)
      dx=-20; // line=nlines/2;
    //else
    if (geq.subtype!=_SEQ__VECT){
      line=0;
      listormat=1;
      if (ckmatrix(geq)){
	ncols=geq._VECTptr->front()._VECTptr->size();
	bool big=eqdata.dy>=LCD_HEIGHT_PX-STATUS_AREA_PX;
	if (big)
	  dy=eqdata.y+eqdata.dy+32;
	col=0;
	line=giacmin(4,nlines/2);
	listormat=2;
      }
    }
  }
  if (!listormat){
    xcas::Equation_select(eq.data,true);
    xcas::eqw_select_down(eq.data);
  }
  //cout << eq.data << endl;
  int firstrun=2;
  for (;;){
#if 1
    if (firstrun==2){
      DefineStatusMessage((char*)(lang?"EXE: quitte, resultat dans last":"EXE: quit, result stored in last"), 1, 0, 0);
      //EnableStatusArea(2);
      DisplayStatusArea();
      firstrun=1;
    }
    else
      set_xcas_status();
#else
    DefineStatusMessage((char*)"+-: zoom, pad: move, EXIT: quit", 1, 0, 0);
    //EnableStatusArea(2);
    DisplayStatusArea();
#endif
    gen value;
    if (listormat) // select line l, col c
      xcas::eqw_select(eq.data,line,col,true,value);
#define EQW_TAILLE 54
    if (eqdata.dx>LCD_WIDTH_PX){
      if (dx<-EQW_TAILLE)
	dx=-EQW_TAILLE;
      if (dx>eqdata.dx-LCD_WIDTH_PX+EQW_TAILLE)
	dx=eqdata.dx-LCD_WIDTH_PX+EQW_TAILLE;
    }
    if (eqdata.dy>LCD_HEIGHT_PX-EQW_TAILLE){
      if (dy-eqdata.y<LCD_HEIGHT_PX-EQW_TAILLE)
	dy=eqdata.y+LCD_HEIGHT_PX-EQW_TAILLE;
      if (dy-eqdata.y>eqdata.dy+EQW_TAILLE)
	dy=eqdata.y+eqdata.dy+EQW_TAILLE;
    }
    drawRectangle(0, STATUS_AREA_PX, LCD_WIDTH_PX, LCD_HEIGHT_PX-STATUS_AREA_PX,COLOR_WHITE);
    // Bdisp_AllClr_VRAM();
    int save_clip_ymin=clip_ymin;
    clip_ymin=STATUS_AREA_PX;
    xcas::display(eq,dx,dy);
    // PrintMini(0,58,menu.c_str(),4);
    //draw_menu(2);
    clip_ymin=save_clip_ymin;
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    if (firstrun){ // workaround for e.g. 1+x/2 partly not displayed
      firstrun=0;
      continue;
    }
    string menu(" "),shiftmenu,alphamenu;
    int eqwcolorbg;
    get_current_console_menu(menu,shiftmenu,alphamenu,eqwcolorbg,2);
    casiostatus(0,0,0,0);
    int key;
    in_ckgetkey(&key,1,menu.c_str(),shiftmenu.c_str(),alphamenu.c_str(),eqwcolorbg);
    bool alph=oldalphastate;//keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
    //cout << key << '\n';
    if (key==KEY_CTRL_SD){
      khicas_addins_menu(contextptr);
      continue;
    }
    if (key==KEY_CTRL_INS){
      key=chartab();
      if (key<0)
	continue;
    }
    if (key==KEY_CTRL_UNDO){
      giac::swapgen(eq.undodata,eq.data);
      if (listormat){
	xcas::do_select(eq.data,true,value);
	if (value.type==_EQW){
	  gen g=eval(value._EQWptr->g,1,contextptr);
	  if (g.type==_VECT){
	    const vecteur & v=*g._VECTptr;
	    nlines=v.size();
	    if (line >= nlines)
	      line=nlines-1;
	    if (col!=-1 &&v.front().type==_VECT){
	      ncols=v.front()._VECTptr->size();
	      if (col>=ncols)
		col=ncols-1;
	    }
	    xcas::do_select(eq.data,false,value);
	    xcas::eqw_select(eq.data,line,col,true,value);
	  }
	}
      }
      continue;
    }
    if (key==KEY_CTRL_F5){
      handle_f5();
      continue;
    }
    if (key=='=' )
      continue;
    int redo=0;
    if (listormat){
      if (key==KEY_CHAR_COMMA || key==KEY_CTRL_DEL ){
	xcas::do_select(eq.data,true,value);
	if (value.type==_EQW){
	  gen g=eval(value._EQWptr->g,1,contextptr);
	  if (g.type==_VECT){
	    edited=true; eq.undodata=xcas::Equation_copy(eq.data);
	    vecteur v=*g._VECTptr;
	    if (key==KEY_CHAR_COMMA){
	      if (col==-1 || (line>0 && line==nlines-1)){
		v.insert(v.begin()+line+1,0*v.front());
		++line; ++nlines;
	      }
	      else {
		v=mtran(v);
		v.insert(v.begin()+col+1,0*v.front());
		v=mtran(v);
		++col; ++ncols;
	      }
	    }
	    else {
	      if (col==-1 || (nlines>=3 && line==nlines-1)){
		if (nlines>=(col==-1?2:3)){
		  v.erase(v.begin()+line,v.begin()+line+1);
		  if (line) --line;
		  --nlines;
		}
	      }
	      else {
		if (ncols>=2){
		  v=mtran(v);
		  v.erase(v.begin()+col,v.begin()+col+1);
		  v=mtran(v);
		  if (col) --col; --ncols;
		}
	      }
	    }
	    geq=gen(v,g.subtype);
	    key=0; redo=1;
	    // continue;
	  }
	}
      }
    }
    bool ins=key==KEY_CHAR_STORE  || key==KEY_CHAR_RPAR || key==KEY_CHAR_LPAR || key==KEY_CHAR_COMMA || key==KEY_CTRL_PASTE;
    int xleft,ytop,xright,ybottom,gselpos; gen * gsel=0,*gselparent=0;
    ustl::string varname;
    if (key==KEY_CTRL_CLIP){
      xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,0);
      if (gsel==0)
	gsel==&eq.data;
      // cout << "var " << g << " " << eq.data << endl;
      if (xcas::do_select(*gsel,true,value) && value.type==_EQW){
	//cout << g << ":=" << value._EQWptr->g << endl;
	copy_clipboard(value._EQWptr->g.print(contextptr),true,true);
	continue;
      }
    }
    if (key==KEY_CHAR_STORE){
      int keyflag = GetSetupSetting( (unsigned int)0x14);
      if (keyflag==0)
	handle_f5();
      if (inputline(lang?"Stocker selection dans":"Save selection in",lang?"Nom de variable: ":"Variable name: ",varname,false) && !varname.empty() && isalpha(varname[0])){
	giac::gen g(varname,contextptr);
	giac::gen ge(eval(g,1,contextptr));
	if (g.type!=_IDNT){
	  invalid_varname();
	  continue;
	}
	if (ge==g || confirm_overwrite()){
	  vector<int> goto_sel;
	  xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel);
	  if (gsel==0)
	    gsel==&eq.data;
	  // cout << "var " << g << " " << eq.data << endl;
	  if (xcas::do_select(*gsel,true,value) && value.type==_EQW){
	    //cout << g << ":=" << value._EQWptr->g << endl;
	    giac::gen gg(value._EQWptr->g);
	    if (gg.is_symb_of_sommet(at_makevector))
	      gg=giac::eval(gg,1,contextptr);
	    giac::sto(gg,g,contextptr);
	  }
	}
      }
      continue;
    }
    if (key==KEY_CTRL_DEL){
      vector<int> goto_sel;
      if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel && xcas::do_select(*gsel,true,value) && value.type==_EQW){
	value=value._EQWptr->g;
	if (value.type==_SYMB){
	  gen tmp=value._SYMBptr->feuille;
	  if (tmp.type!=_VECT || tmp.subtype!=_SEQ__VECT){
	    xcas::replace_selection(eq,tmp,gsel,&goto_sel);
	    continue;
	  }
	}
	if (!goto_sel.empty() && gselparent && gselparent->type==_VECT && !gselparent->_VECTptr->empty()){
	  vecteur & v=*gselparent->_VECTptr;
	  if (v.back().type==_EQW){
	    gen opg=v.back()._EQWptr->g;
	    if (opg.type==_FUNC){
	      int i=0;
	      for (;i<v.size()-1;++i){
		if (&v[i]==gsel)
		  break;
	      }
	      if (i<v.size()-1){
		if (v.size()==5 && (opg==at_integrate || opg==at_sum) && i>=2)
		  v.erase(v.begin()+2,v.begin()+4);
		else
		  v.erase(v.begin()+i);
		xcas::do_select(*gselparent,true,value);
		if (value.type==_EQW){
		  value=value._EQWptr->g;
		  // cout << goto_sel << " " << value << endl; continue;
		  if (v.size()==2 && (opg==at_plus || opg==at_prod || opg==at_pow))
		    value=eval(value,1,contextptr);
		  goto_sel.erase(goto_sel.begin());
		  xcas::replace_selection(eq,value,gselparent,&goto_sel);
		  continue;
		}
	      }
	    }
	  }
	}
      }
    }
    if (key==KEY_CTRL_F9){
      key=KEY_CTRL_F3;
      keyflag=1;
    }
    if (key==KEY_CTRL_F12){
      key=KEY_CTRL_F6;
      keyflag=1;
    }
    if (key==KEY_CTRL_F3){
      if (keyflag==1){
	xcas::do_select(eq.data,true,value);
	if (value.type==_EQW)
	  geq=value._EQWptr->g;
	if (eq.attr.fontsize==14)
	  eq.attr.fontsize=16;
	else {
	  if (eq.attr.fontsize>=18)
	    eq.attr.fontsize=14;
	  else
	    eq.attr.fontsize=18;
	}
	redo=1;
      }
      else {
	if (alph){
	  if (eq.attr.fontsize<=14) continue;
	  xcas::do_select(eq.data,true,value);
	  if (value.type==_EQW)
	    geq=value._EQWptr->g;
	  if (eq.attr.fontsize==16)
	    eq.attr.fontsize=14;
	  else
	    eq.attr.fontsize=16;
	  redo=1;
	}
	else {
	  // Edit
	  edited=true;
	  ins=true;
	}
      }
    }
    if (key==KEY_CHAR_IMGNRY)
      key='i';
    const char keybuf[2]={(key==KEY_CHAR_PMINUS?'-':char(key)),0};
    translate_fkey(key);
    const char * adds=(key==KEY_CHAR_PMINUS ||
		       (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F20) ||
		       (key==char(key) && (isalphanum(key)|| key=='.' ))
		       )?keybuf:keytostring(key,keyflag);
    if ( key==KEY_CTRL_F1 || key==KEY_CTRL_F2 
	 || (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F20)
	 ){
      adds=console_menu(key,1);//alph?"simplify":(keyflag==1?"factor":"partfrac");
      if (!adds) continue;
      // workaround for infinitiy
      if (strlen(adds)>=2 && adds[0]=='o' && adds[1]=='o')
	key=KEY_CTRL_F5;      
    }
    if (key==KEY_CTRL_F6){
      adds=alph?"regroup":(keyflag==1?"evalf":"eval");
    }
    if (key==KEY_CTRL_F5)
      adds="oo";
    if (key==KEY_CHAR_MINUS)
      adds="-";
    if (key==KEY_CHAR_EQUAL)
      adds="=";
    if (key==KEY_CHAR_RECIP)
      adds="inv";
    if (key==KEY_CHAR_SQUARE)
      adds="sq";
    if (key==KEY_CHAR_POWROOT)
      adds="surd";
    if (key==KEY_CHAR_CUBEROOT)
      adds="surd";
    int addssize=adds?strlen(adds):0;
    // cout << addssize << " " << adds << endl;
    if (key==KEY_CTRL_EXE){
      if (xcas::do_select(eq.data,true,value) && value.type==_EQW){
	//cout << "ok " << value._EQWptr->g << endl;
	//DefineStatusMessage((char*)lang?"resultat stocke dans last":"result stored in last", 1, 0, 0);
	//DisplayStatusArea();
	giac::sto(value._EQWptr->g,giac::gen("last",contextptr),contextptr);
	return value._EQWptr->g;
      }
      //cout << "no " << eq.data << endl; if (value.type==_EQW) cout << value._EQWptr->g << endl ;
      return geq;
    }
    if ( key!=KEY_CHAR_MINUS && key!=KEY_CHAR_EQUAL &&
	(ins || key==KEY_CHAR_PI || key==KEY_CTRL_F5 || (addssize==1 && (isalphanum(adds[0])|| adds[0]=='.' || adds[0]=='-') ) )
	){
      edited=true;
      if (line>=0 && xcas::eqw_select(eq.data,line,col,true,value)){
	ustl::string s;
	if (ins){
	  if (key==KEY_CTRL_PASTE)
	    s=paste_clipboard();
	  else {
	    if (value.type==_EQW){
	      s=value._EQWptr->g.print(contextptr);
	    }
	    else
	      s=value.print(contextptr);
	  }
	}
	else
	  s = adds;
 	ustl::string msg("Line ");
	msg += print_INT_(line+1);
	msg += " Col ";
	msg += print_INT_(col+1);
	if (inputline(msg.c_str(),0,s,false)==KEY_CTRL_EXE){
	  value=gen(s,contextptr);
	  if (col<0)
	    (*geq._VECTptr)[line]=value;
	  else
	    (*((*geq._VECTptr)[line]._VECTptr))[col]=value;
	  redo=2;
	  key=KEY_SHIFT_RIGHT;
	}
	else
	  continue;
      }
      else {
	vector<int> goto_sel;
	if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel && xcas::do_select(*gsel,true,value) && value.type==_EQW){
	  ustl::string s;
	  if (ins){
	    if (key==KEY_CTRL_PASTE)
	      s=paste_clipboard();
	    else {
	      s = value._EQWptr->g.print(contextptr);
	      if (key==KEY_CHAR_COMMA)
		s += ',';
	    }
	  }
	  else
	    s = adds;
	  if (inputline(value._EQWptr->g.print(contextptr).c_str(),0,s,false)==KEY_CTRL_EXE){
	    value=gen(s,contextptr);
	    //cout << value << " goto " << goto_sel << endl;
	    xcas::replace_selection(eq,value,gsel,&goto_sel);
	    firstrun=-1; // workaround, force 2 times display
	  }
	  continue;
	}
      }
    }
    if (redo){
      eq.data=0; // clear memory
      eq.data=xcas::Equation_compute_size(geq,eq.attr,LCD_WIDTH_PX,contextptr);
      eqdata=xcas::Equation_total_size(eq.data);
      if (redo==1){
	dx=(eqdata.dx-LCD_WIDTH_PX)/2;
	dy=LCD_HEIGHT_PX-2*STATUS_AREA_PX+eqdata.y;
	if (listormat) // select line l, col c
	  xcas::eqw_select(eq.data,line,col,true,value);
	else {
	  xcas::Equation_select(eq.data,true);
	  xcas::eqw_select_down(eq.data);
	}
	continue;
      }
    }
    if (key==KEY_CTRL_EXIT || key==KEY_CTRL_AC){
      if (!edited)
	return geq;
      if (confirm(lang?"Vraiment abandonner?":"Really leave",lang?"F1: retour editeur,  F6: confirmer":"F1: back to editor,  F6: confirm")==KEY_CTRL_F6)
	return undef;
    }
    bool doit=eqdata.dx>=LCD_WIDTH_PX;
    int delta=0;
    if (listormat){
      if (key==KEY_CTRL_LEFT  || (!doit && key==KEY_SHIFT_LEFT)){
	if (line>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	  if (col>=0){
	    --col;
	    if (col<0){
	      col=ncols-1;
	      if (line>0)
		--line;
	    }
	  }
	  else {
	    if (line>0)
	      --line;
	  }
	  xcas::eqw_select(eq.data,line,col,true,value);
	  if (doit) dx -= value._EQWptr->dx;
	}
	continue;
      }
      if (doit && key==KEY_SHIFT_LEFT){
	dx -= 20;
	continue;
      }
      if (key==KEY_CTRL_RIGHT  || (!doit && key==KEY_SHIFT_RIGHT)) {
	if (line>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	  if (doit)
	    dx += value._EQWptr->dx;
	  if (col>=0){
	    ++col;
	    if (col==ncols){
	      col=0;
	      if (line<nlines-1)
		++line;
	    }
	  } else {
	    if (line<nlines-1)
	      ++line;
	  }
	  xcas::eqw_select(eq.data,line,col,true,value);
	}
	continue;
      }
      if (key==KEY_SHIFT_RIGHT && doit){
	dx += 20;
	continue;
      }
      doit=eqdata.dy>=LCD_HEIGHT_PX-3*STATUS_AREA_PX;
      if (key==KEY_CTRL_UP || (!doit && key==KEY_CTRL_PAGEUP)){
	if (line>0 && col>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	  --line;
	  xcas::eqw_select(eq.data,line,col,true,value);
	  if (doit)
	    dy += value._EQWptr->dy+eq.attr.fontsize/2;
	}
        else
          dy += eq.attr.fontsize/2;
	continue;
      }
      if (key==KEY_CTRL_PAGEUP && doit){
	dy += 20;
	continue;
      }
      if (key==KEY_CTRL_DOWN  || (!doit && key==KEY_CTRL_PAGEDOWN)){
	if (line<nlines-1 && col>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	  if (doit)
	    dy -= value._EQWptr->dy+eq.attr.fontsize/2;
          else
            dy -= eq.attr.fontsize/2;
	  ++line;
	  xcas::eqw_select(eq.data,line,col,true,value);
	}
	continue;
      }
      if ( key==KEY_CTRL_PAGEDOWN && doit){
	dy -= 20;
	continue;
      }
    }
    else { // else listormat
      if (key==KEY_CTRL_LEFT){
	delta=xcas::eqw_select_leftright(eq,true,alph?2:0);
	// cout << "left " << delta << endl;
	if (doit) dx += (delta?delta:-20);
	continue;
      }
      if (key==KEY_SHIFT_LEFT){
	delta=xcas::eqw_select_leftright(eq,true,1);
	vector<int> goto_sel;
	if (doit) dx += (delta?delta:-20);
	continue;
      }
      if (key==KEY_CTRL_RIGHT){
	delta=xcas::eqw_select_leftright(eq,false,alph?2:0);
	// cout << "right " << delta << endl;
	if (doit)
	  dx += (delta?delta:20);
	continue;
      }
      if (key==KEY_SHIFT_RIGHT){
	delta=xcas::eqw_select_leftright(eq,false,1);
	// cout << "right " << delta << endl;
	if (doit)
	  dx += (delta?delta:20);
	// dx=eqdata.dx-LCD_WIDTH_PX+20;
	continue;
      }
      doit=eqdata.dy>=LCD_HEIGHT_PX-2*STATUS_AREA_PX;
      if (key==KEY_CTRL_UP){
	delta=xcas::eqw_select_up(eq.data);
	// cout << "up " << delta << endl;
	continue;
      }
      //cout << "up " << eq.data << endl;
      if (key==KEY_CTRL_PAGEUP && doit){
	dy=eqdata.y+eqdata.dy+20;
	continue;
      }
      if (key==KEY_CTRL_DOWN){
	delta=xcas::eqw_select_down(eq.data);
	// cout << "down " << delta << endl;
	continue;
      }
      //cout << "down " << eq.data << endl;
      if ( key==KEY_CTRL_PAGEDOWN && doit){
	dy=eqdata.y+LCD_HEIGHT_PX-STATUS_AREA_PX;
	continue;
      }
    }
    if (adds){
      edited=true;
      if (strcmp(adds,"f(x):=")==0 || strcmp(adds,":=")==0)
	adds="f";
      if (strcmp(adds,"'")==0)
	adds="diff";
      if (strcmp(adds,"^2")==0)
	adds="sq";
      if (strcmp(adds,">")==0)
	adds="simplify";
      if (strcmp(adds,"<")==0)
	adds="factor";
      if (strcmp(adds,"#")==0)
	adds="partfrac";
      string cmd(adds);
      if (cmd.size() && cmd[cmd.size()-1]=='(')
	cmd ='\''+cmd.substr(0,cmd.size()-1)+'\'';
      vector<int> goto_sel;
      if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel){
	gen op;
	int addarg=0;
	if (addssize==1){
	  switch (adds[0]){
	  case '+':
	    addarg=1;
	    op=at_plus;
	    break;
	  case '^':
	    addarg=1;
	    op=at_pow;
	    break;
	  case '=':
	    addarg=1;
	    op=at_equal;
	    break;
	  case '-':
	    addarg=1;
	    op=at_binary_minus;
	    break;
	  case '*':
	    addarg=1;
	    op=at_prod;
	    break;
	  case '/':
	    addarg=1;
	    op=at_division;
	    break;
	  case '\'':
	    addarg=1;
	    op=at_diff;
	    break;
	  }
	}
	if (op==0)
	  op=gen(cmd,contextptr);
	if (op.type==_SYMB)
	  op=op._SYMBptr->sommet;
	// cout << "keyed " << adds << " " << op << " " << op.type << endl;
	if (op.type==_FUNC){
	  edited=true;
	  // execute command on selection
	  gen tmp,value;
	  if (xcas::do_select(*gsel,true,value) && value.type==_EQW){
	    if (op==at_integrate || op==at_sum)
	      addarg=3;
	    if (op==at_limit)
	      addarg=2;
	    gen args=eval(value._EQWptr->g,1,contextptr);
	    gen vx=xthetat?t__IDNT_e:x__IDNT_e;
	    if (addarg==1)
	      args=makesequence(args,0);
	    if (addarg==2)
	      args=makesequence(args,vx_var,0);
	    if (addarg==3)
	      args=makesequence(args,vx_var,0,1);
	    if (op==at_surd)
	      args=makesequence(args,key==KEY_CHAR_CUBEROOT?3:4);
	    if (op==at_subst)
	      args=makesequence(args,giac::symb_equal(vx_var,0));
	    unary_function_ptr immediate_op[]={*at_eval,*at_evalf,*at_evalc,*at_regrouper,*at_simplify,*at_normal,*at_ratnormal,*at_factor,*at_cfactor,*at_partfrac,*at_cpartfrac,*at_expand,*at_canonical_form,*at_exp2trig,*at_trig2exp,*at_sincos,*at_lin,*at_tlin,*at_tcollect,*at_texpand,*at_trigexpand,*at_trigcos,*at_trigsin,*at_trigtan,*at_halftan};
	    if (equalposcomp(immediate_op,*op._FUNCptr)){
	      set_abort();
	      tmp=(*op._FUNCptr)(args,contextptr);
	      clear_abort();
	      esc_flag=0;
	      ctrl_c=false;
	      kbd_interrupted=interrupted=false;
	    }
	    else
	      tmp=symbolic(*op._FUNCptr,args);
	    //cout << "sel " << value._EQWptr->g << " " << tmp << " " << goto_sel << endl;
	    esc_flag=0;
	    ctrl_c=false;
	    kbd_interrupted=interrupted=false;
	    if (!is_undef(tmp)){
	      xcas::replace_selection(eq,tmp,gsel,&goto_sel);
	      if (addarg){
		xcas::eqw_select_down(eq.data);
		xcas::eqw_select_leftright(eq,false);
	      }
	      eqdata=xcas::Equation_total_size(eq.data);
	      dx=(eqdata.dx-LCD_WIDTH_PX)/2;
	      dy=LCD_HEIGHT_PX-2*STATUS_AREA_PX+eqdata.y;
	      firstrun=-1; // workaround, force 2 times display
	    }
	  }
	}
      }
    }
  }
  //*logptr(contextptr) << eq.data << endl;
}

bool textedit(char * s){
  if (!s)
    return false;
  int ss=strlen(s);
  if (ss==0){
    *s=' ';
    s[1]=0;
    ss=1;
  }
  textArea ta;
  ta.elements.clear();
  ta.editable=true;
  ta.clipline=-1;
  ta.changed=false;
  ta.filename="\\\\fls0\\temp.py";
  ta.y=0;
  ta.allowEXE=true;
  bool str=s[0]=='"' && s[ss-1]=='"';
  if (str){
    s[ss-1]=0;
    add(&ta,s+1);
  }
  else
    add(&ta,s);
  ta.line=0;
  ta.pos=ta.elements[ta.line].s.size();
  int res=doTextArea(&ta);
  if (res==TEXTAREA_RETURN_EXIT)
    return false;
  string S(merge_area(ta.elements));
  if (str)
    S='"'+S+'"';
  int Ssize=S.size();
  if (Ssize<GEN_PRINT_BUFSIZE){
    strcpy(s,S.c_str());
    for (--Ssize;Ssize>=0;--Ssize){
      if ((unsigned char)s[Ssize]==0x9c || s[Ssize]=='\n')
	s[Ssize]=0;
      if (s[Ssize]!=' ')
	break;
    }
    return true;
  }
  return false;
}

bool textedit(char * s,int bufsize,bool OKparse,const giac::context * contextptr,const char * filename){
  if (!s)
    return false;
  int ss=strlen(s);
  if (ss==0){
    *s=' ';
    s[1]=0;
    ss=1;
  }
  textArea ta;
  ta.elements.clear();
  ta.editable=true;
  ta.clipline=-1;
  ta.changed=false;
  ta.filename=filename?filename:"temp.py";
  ta.y=0;
  ta.python=python_compat(contextptr);
  ta.allowEXE=false;//true; // set back to true later
  ta.OKparse=OKparse;
  bool str=s[0]=='"' && s[ss-1]=='"';
  if (str){
    s[ss-1]=0;
    add(&ta,s+1);
  }
  else
    add(&ta,s);
  ta.line=0;
  ta.pos=ta.elements[ta.line].s.size();
  int res=doTextArea(&ta);
  drawRectangle(0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX,_WHITE);
  // os_hide_graph();
  if (res==TEXTAREA_RETURN_EXIT)
    return false;
  string S(merge_area(ta.elements));
  if (str)
    S='"'+S+'"';
  int Ssize=S.size();
  if (Ssize<bufsize){
    strcpy(s,S.c_str());
    for (--Ssize;Ssize>=0;--Ssize){
      if ((unsigned char)s[Ssize]==0x9c || s[Ssize]=='\n')
	s[Ssize]=0;
      if (s[Ssize]!=' ')
	break;
    }
    return true;
  }
  return false;
}

bool textedit(char * s,int bufsize,const giac::context * contextptr){
  return textedit(s,bufsize,false,contextptr);
}

static string cascas_lower_ascii(const string &s){
  string out;
  for (int i=0;i<int(s.size());++i)
    out += char(tolower((unsigned char)s[i]));
  return out;
}

static string cascas_compact_ascii(const string &s){
  string out;
  for (int i=0;i<int(s.size());++i)
    if (!isspace((unsigned char)s[i]) && s[i]!='*')
      out += char(tolower((unsigned char)s[i]));
  return out;
}

static string cascas_trim_ascii(const string &s){
  int a=0,b=s.size();
  while (a<b && isspace((unsigned char)s[a])) ++a;
  while (b>a && isspace((unsigned char)s[b-1])) --b;
  return s.substr(a,b-a);
}

static bool cascas_ident_char(char c){
  return isalnum((unsigned char)c) || c=='_';
}

static unsigned cascas_hash_ident(const string &s,int begin,int end){
  unsigned h=2166136261u;
  for (int i=begin;i<end;++i){
    h ^= (unsigned char)s[i];
    h *= 16777619u;
  }
  return h;
}

static bool cascas_denied_hash(unsigned h){
  switch (h){
    case 0x18585f14u:
    case 0x0f477e9au:
    case 0x0e5f3468u:
    case 0xeb64d5d5u:
    case 0x79a98884u:
    case 0x50a09eb9u:
    case 0x0c5bcd9du:
    case 0xaef31c63u:
    case 0xa51be2bbu:
    case 0x0b11b072u:
    case 0x3d8466cbu:
    case 0x64bcb23du:
    case 0xc2ab04e9u:
    case 0x9c436708u:
    case 0x12ca106eu:
    case 0x7a78762fu:
    case 0xc9648178u:
    case 0xb1727e44u:
    case 0x9fff2789u:
    case 0xa48fb4c9u:
    case 0x79a94f04u:
    case 0x0aa7f0b9u:
    case 0x7084d38du:
    case 0x89eabb08u:
    case 0xa01e3d98u:
    case 0xbcefb4d8u:
    case 0xf5cf8c7du:
    case 0xaa7d7949u:
    case 0x11c2662du:
    case 0xdb9215fdu:
    case 0x4ceb35d5u:
    case 0xcb08e2d8u:
    case 0x8117ae3du:
    case 0xfcaee333u:
    case 0xc7e16877u:
    case 0x917c0699u:
    case 0xd686c6a1u:
    case 0x1100db48u:
    case 0x9ede2954u:
    case 0x14cdb897u:
    case 0x41229629u:
    case 0x0dd0b0beu:
    case 0xd7599ae2u:
    case 0xc1c5c6a2u:
    case 0x5319fe9eu:
    case 0x1427b873u:
    case 0xae13f94au:
    case 0x50b13859u:
    case 0x2f271707u:
    case 0x9c662fa4u:
    case 0xd6f47b66u:
    case 0xa19b8cd6u:
    case 0xa9cba3f7u:
    case 0x1bbf4029u:
    case 0x8f9b7080u:
    case 0xaa9b9b01u:
    case 0x6f20f7ddu:
    case 0xe667368du:
    case 0x178d4c35u:
    case 0x8e9dd543u:
    case 0xd04036f3u:
    case 0x4038790bu:
    case 0x58547550u:
    case 0x4edf1404u:
    case 0x10d2583fu:
    case 0xf45c461cu:
    case 0x092855d0u:
    case 0xbab19e4au:
    case 0xedf2c855u:
    case 0x07275075u:
    case 0x0fceff97u:
    case 0x99b5d744u:
    case 0x830223f4u:
    case 0x616bf796u:
    case 0x8c496ab8u:
    case 0xbf5074c1u:
    case 0xf160b465u:
    case 0x15c2f8ecu:
    case 0xfe42bdf4u:
    case 0xb0f0336bu:
    case 0x1d11622cu:
    case 0x23c6e902u:
    case 0x08230495u:
    case 0xeea7e9ccu:
    case 0x00313120u:
    case 0x91708e0cu:
    case 0xf78714e6u:
    case 0x2ba548a6u:
    case 0x45855c00u:
    case 0xd4f0716cu:
    case 0xd4ada34au:
    case 0x614ba208u:
    case 0xa170a73cu:
    case 0x13c6cfd2u:
    case 0x3ba561d6u:
    case 0x4a8d76c1u:
    case 0x4f04d9d2u:
    case 0x547835f3u:
    case 0xb5fc9604u:
    case 0xa1ed81c2u:
    case 0xeb2af50bu:
    case 0x9ebaa335u:
    case 0xfceb725bu:
    case 0x0073dacfu:
    case 0x5c95b7aeu:
    case 0x2bd14411u:
    case 0xea8bdf11u:
    case 0x3ca7ff7eu:
    case 0x94b9211bu:
    case 0x9c265311u:
    case 0x62e5b688u:
    case 0x3bea1b45u:
    case 0x6ce3e74du:
    case 0xc16374f9u:
    case 0xdef6fa2au:
    case 0xb5bda71fu:
    case 0x70ffec21u:
    case 0xc59c1257u:
    case 0xce1444dau:
    case 0xd1b79688u:
    case 0x0dc628ceu:
    case 0xacf38390u:
    case 0x85ee37bfu:
    case 0xf9d86f7bu:
    case 0xcedfa3c5u:
    case 0xbe269f5cu:
    case 0x46b93feau:
    case 0xe49f78d1u:
    case 0x7a0e47bau:
    case 0x02dc2b01u:
    case 0x68365378u:
    case 0xeb40ffd3u:
    case 0x2b8eb358u:
    case 0x8f605233u:
    case 0x8bfaffcfu:
    case 0x1ebe2c6au:
    case 0x1413edcdu:
    case 0x17d1d6fcu:
    case 0xd7037e9bu:
    case 0x633cdd81u:
    case 0xd2353873u:
    case 0x38185d76u:
    case 0xa3ad2dc9u:
    case 0xe272ae1bu:
    case 0xb9c6a953u:
    case 0xbbc6ac79u:
    case 0x7bc647b9u:
    case 0xab66535cu:
    case 0x31bd46a1u:
    case 0xbdeeca89u:
    case 0xc7bd6948u:
    case 0xd4d5660fu:
    case 0x65c08844u:
    case 0x9e99b437u:
    case 0x69e3e294u:
    case 0x24fa4703u:
    case 0x22fc8274u:
    case 0x04fe91d1u:
    case 0xf3fa9284u:
    case 0x49f2dba0u:
    case 0x6cf8e81cu:
    case 0x102c19eau:
    case 0xac5195bau:
    case 0xb4e7f8e4u:
    case 0xb469c380u:
    case 0xacda80fbu:
    case 0xec7aec99u:
    case 0x1b5e2979u:
    case 0x8e816a60u:
    case 0xcc30f8fau:
    case 0x1696d742u:
    case 0x856d7f49u:
    case 0x1eab292au:
    case 0x071515d2u:
    case 0xa71467f9u:
    case 0x9c90b906u:
    case 0x7d6fc026u:
    case 0x5d36cd35u:
    case 0x5824dcf7u:
    case 0xd51ec1fdu:
    case 0xc109695bu:
    case 0xad7f573cu:
    case 0x6e12076fu:
    case 0xa8aef257u:
    case 0x78fda3ffu:
    case 0xfbf05517u:
    case 0xd46fb0cfu:
    case 0xf2c80d5bu:
    case 0x98a2959eu:
    case 0x6ef4d9bdu:
    case 0xe26424d0u:
    case 0x7d033966u:
    case 0xd81f50a6u:
    case 0x1b777caeu:
    case 0x6c04a192u:
    case 0x826fd909u:
    case 0x1f949aa9u:
    case 0xa21ca480u:
    case 0xea42b557u:
    case 0x4c515decu:
    case 0xbde0f6e0u:
    case 0x78b0ef32u:
    case 0x0ad0ed6cu:
    case 0x89b28834u:
    case 0x9b4bf7b0u:
    case 0x28c49dd1u:
    case 0xb760552au:
    case 0x83092c31u:
    case 0x11a3508au:
    case 0x6371dcb0u:
    case 0xd20a0c9du:
    case 0xdf9e7283u:
    case 0x28a080d8u:
    case 0xdb6d3576u:
    case 0x0cbc8ba4u:
    case 0x46574072u:
    case 0x2272d6aau:
    case 0xa84c031du:
    case 0x18ae6c91u:
    case 0x17db1627u:
    case 0xf3c342e6u:
    case 0x28217089u:
    case 0xceaa3082u:
    case 0x318b991du:
    case 0x70c17171u:
    case 0x6fc16fdeu:
    case 0xdd404b6au:
    case 0x6b91d120u:
    case 0xad1ebfefu:
    case 0xf2e361a3u:
    case 0x43190e92u:
    case 0x89a905a4u:
    case 0xafd98518u:
    case 0xfccd1337u:
    case 0x2ee0698fu:
    case 0xcccb0055u:
    case 0xf3a20824u:
    case 0x9e888b98u:
    case 0xd4cbbf07u:
    case 0x51295e73u:
    case 0x64f89bc6u:
    case 0x503e3086u:
    case 0x372f4f55u:
    case 0xd5b7b015u:
    case 0x0fad7a44u:
    case 0x198be6b8u:
    case 0xdd4a6ba7u:
    case 0xee43b913u:
    case 0xbda420c0u:
    case 0x2d5f622cu:
    case 0x626e3204u:
    case 0xaf645378u:
    case 0x46e55a67u:
    case 0xf8aaffd3u:
    case 0x1e21273au:
    case 0xb9edf782u:
    case 0x67960d89u:
    case 0xf28765e9u:
    case 0x1642b069u:
    case 0xb4423af0u:
    case 0x05804268u:
    case 0x0e684b5cu:
    case 0xe35093bfu:
    case 0xde375a53u:
    case 0xe67c786cu:
    case 0xe02a462fu:
    case 0x940b5e09u:
    case 0xebd12f63u:
    case 0xd1b1e431u:
    case 0x9450ab6au:
    case 0xa450c49au:
    case 0x65cc8d1bu:
    case 0x66cc8eaeu:
    case 0x6acc94fau:
    case 0x262cfcabu:
    case 0xccee2183u:
    case 0x81df5ea5u:
    case 0xcc8d8dedu:
    case 0x8bd5ceabu:
    case 0x7d3ff927u:
    case 0x6eb56479u:
    case 0x7e8a3329u:
    case 0x5e7f5371u:
    case 0xe89516b0u:
    case 0xe83e3398u:
    case 0x18ba068bu:
    case 0x05f85713u:
    case 0x4810513cu:
    case 0x144f0c62u:
    case 0x8dffe37bu:
    case 0x66bb20b3u:
    case 0x9a0f9c08u:
    case 0x5683a798u:
    case 0xd6b186b8u:
    case 0x6a7d2c89u:
    case 0x880dac7fu:
    case 0xc3e8e5f7u:
    case 0xd4f67cfdu:
    case 0x4172a512u:
    case 0xc2eb0145u:
    case 0xe44789e8u:
    case 0x7315d9bdu:
    case 0x229e4ed8u:
    case 0xac9b9e27u:
    case 0xdd1afb2fu:
    case 0xb66f5021u:
    case 0x43831c98u:
    case 0x1f82d9a8u:
    case 0x15351b6du:
    case 0xb35135fau:
    case 0xf752b052u:
    case 0x42f48402u:
    case 0xb323923eu:
    case 0x8a2c68b7u:
    case 0x3508aee0u:
    case 0x1832aa87u:
    case 0x47f5473cu:
    case 0x4ed10951u:
    case 0x813d75aeu:
    case 0xc03183c0u:
    case 0xfb3d7a15u:
    case 0x660d7050u:
    case 0xae96da47u:
    case 0x80945346u:
    case 0x0f9079dbu:
    case 0x1809966eu:
    case 0xab54ea82u:
    case 0x77b7a398u:
    case 0x46544626u:
    case 0x14f25eb6u:
    case 0xe145ee5du:
    case 0xb7f23106u:
    case 0x9ef2cbdcu:
    case 0x0cbf3400u:
    case 0xdbf7eff4u:
    case 0xb1e15f65u:
    case 0x97736dd1u:
    case 0x8d2519cbu:
    case 0xfd455c3bu:
    case 0xfa10a755u:
    case 0x9311bc11u:
    case 0x8fae83d8u:
    case 0xfba40b66u:
    case 0xa3325c6fu:
    case 0xb1c0e744u:
    case 0x19f1a786u:
    case 0x519b7f08u:
    case 0xee65cbdau:
    case 0xb3dab835u:
    case 0xf3c7dd11u:
    case 0x4d19907fu:
    case 0x3f29a48cu:
    case 0xdee6ee22u:
    case 0x98924db9u:
      return true;
    default:
      return false;
  }
}

static int cascas_find_matching_paren_ascii(const string &s,int open);

static bool cascas_allowed_binomial_series_call(const string &s,int open){
  int close=cascas_find_matching_paren_ascii(s,open);
  if (close<0)
    return false;
  int depth=0,commas=0;
  bool instring=false;
  for (int i=open+1;i<close;++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c==',') ++commas;
  }
  return commas==3;
}

static bool cascas_reject_removed_feature(const char *input){
  if (!input)
    return false;
  string s=cascas_lower_ascii(input);
  if (s.find('%')!=string::npos || s.find("[[")!=string::npos)
    return true;
  for (int i=0;i<int(s.size());){
    if (!cascas_ident_char(s[i])){
      ++i;
      continue;
    }
    int begin=i;
    while (i<int(s.size()) && cascas_ident_char(s[i]))
      ++i;
    int j=i;
    while (j<int(s.size()) && (s[j]==' ' || s[j]=='\t'))
      ++j;
    unsigned h=cascas_hash_ident(s,begin,i);
    bool bare=(h==0x0fceff97u || h==0x15c2f8ecu ||
	       h==0xdf9e7283u || h==0x70c17171u || h==0x6fc16fdeu);
    if ((bare || (j<int(s.size()) && s[j]=='(')) && cascas_denied_hash(h)){
      if (h==0x1d11622cu && j<int(s.size()) && s[j]=='(' &&
	  cascas_allowed_binomial_series_call(s,j))
	continue;
      return true;
    }
  }
  return false;
}

static const char *cascas_pure_method_fallback(){
  // Legacy checker marker: "Err: unsupported (not A-level scope)".
  return "Pure method fallback:\n"
         "Unsupported built-in removed from this Pure build.\n"
         "1. Rewrite the question using algebra, trig, logs or calculus.\n"
         "2. Try solve, diff, integrate, range, domain, xform, expand or factor.\n"
         "3. For stats, matrices, plotting, scripts or mechanics, use the calculator app for that topic.";
}

static const char *cascas_general_method_fallback(){
  return "General Pure method:\n"
         "1. Identify the target: simplify, solve, differentiate, integrate, prove, range or domain.\n"
         "2. Rewrite with standard A-level identities and restrictions first.\n"
         "3. Then use solve, diff, integrate, range, domain, xform, expand or factor on the clean sub-step.";
}

static bool cascas_error_like_output(const string &s){
  return s=="undef" || s.find("Error")!=string::npos || s.find("ERROR")!=string::npos ||
         s.find("Syntax")!=string::npos || s.find("Bad Argument")!=string::npos ||
         s.find("Invalid")!=string::npos || s.find("Unable")!=string::npos;
}

static int cascas_find_matching_paren_ascii(const string &s,int open){
  int depth=0;
  bool instring=false;
  for (int i=open;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
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

static int cascas_split_top_args_ascii(const string &s,int begin,int end,string *args,int maxargs){
  int depth=0,start=begin,count=0;
  bool instring=false;
  for (int i=begin;i<=end;++i){
    char c=i<end?s[i]:',';
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c==','){
      if (count<maxargs)
	args[count++]=cascas_trim_ascii(s.substr(start,i-start));
      start=i+1;
    }
  }
  return count;
}

static bool cascas_parse_call_ascii(const char *input,const char *name,string *args,int maxargs,int &count){
  count=0;
  if (!input)
    return false;
  string s(input),low=cascas_lower_ascii(s),n(name);
  if (low.find(n)!=0)
    return false;
  int open=n.size();
  if (open>=int(s.size()) || s[open]!='(')
    return false;
  int close=cascas_find_matching_paren_ascii(s,open);
  if (close<0)
    return false;
  count=cascas_split_top_args_ascii(s,open+1,close,args,maxargs);
  return true;
}

static int cascas_find_top_equal_ascii(const string &s){
  int depth=0;
  bool instring=false;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c=='=')
      return i;
  }
  return -1;
}

static bool cascas_rewrite_input(const char *input,string &rewritten){
  string args[3];
  int count=0;
  if (cascas_parse_call_ascii(input,"implicit_diff",args,3,count) && count>=1){
    string expr=args[0],x=count>=2 && args[1].size()?args[1]:"x",y=count>=3 && args[2].size()?args[2]:"y";
    int eq=cascas_find_top_equal_ascii(expr);
    if (eq>=0){
      string tmp("(");
      tmp += expr.substr(0,eq);
      tmp += ")-(";
      tmp += expr.substr(eq+1);
      tmp += ")";
      expr=tmp;
    }
    rewritten="normal(-diff((";
    rewritten += expr;
    rewritten += "),";
    rewritten += x;
    rewritten += ")/diff((";
    rewritten += expr;
    rewritten += "),";
    rewritten += y;
    rewritten += "))";
    return true;
  }
  if (cascas_parse_call_ascii(input,"diff",args,3,count) && count>=2){
    int eq=cascas_find_top_equal_ascii(args[0]);
    if (eq>=0 && cascas_compact_ascii(args[0]).find("y")!=string::npos){
      string expr("(");
      expr += args[0].substr(0,eq);
      expr += ")-(";
      expr += args[0].substr(eq+1);
      expr += ")";
      string x=args[1].size()?args[1]:"x";
      rewritten="normal(-diff((";
      rewritten += expr;
      rewritten += "),";
      rewritten += x;
      rewritten += ")/diff((";
      rewritten += expr;
      rewritten += "),y))";
      return true;
    }
  }
  return false;
}

void do_run(const char * s,gen & g,gen & ge){
  esc_flag=0;
  ctrl_c=false;
  kbd_interrupted=interrupted=false;
  if (!contextptr)
    contextptr=new giac::context;
  int S0=strlen(s);
  int S=S0<GEN_PRINT_BUFSIZE-1?S0:GEN_PRINT_BUFSIZE-1;
  char buf[GEN_PRINT_BUFSIZE];
  buf[S]=0;
  for (int i=0;i<S;++i){
    char c=s[i];
    if (c==0x1e || c==char(0x9c))
      buf[i]='\n';
    else {
      if (c==0x0d)
	buf[i]=' ';
      else
	buf[i]=c;
    }
  }
  if (cascas_reject_removed_feature(buf)){
    string msg="\"";
    msg += cascas_pure_method_fallback();
    msg += "\"";
    ge=string2gen(msg.c_str(),false);
    return;
  }
  string rewritten;
  if (cascas_rewrite_input(buf,rewritten)){
    S=rewritten.size();
    if (S>=GEN_PRINT_BUFSIZE)
      S=GEN_PRINT_BUFSIZE-1;
    strncpy(buf,rewritten.c_str(),S);
    buf[S]=0;
  }
  g=gen(buf,contextptr);
  //Console_Output(g.print(contextptr).c_str()); return ;
  giac::freeze=false;
  execution_in_progress = 1;
  giac::set_abort();
  ge=eval(equaltosto(g,contextptr),1,contextptr);
  giac::clear_abort();
  execution_in_progress = 0;
  if (esc_flag || ctrl_c){
    while (confirm("Interrupted","F1/F6: ok",true)==-1)
      ; // insure ON has been removed from keyboard buffer
    ge=string2gen("Interrupted",false);
  }
  // memory full?
  if (get_free_memory()<48*1024){
    // clear turtle, display msg
#ifndef TURTLETAB
    turtle_stack().erase(turtle_stack().begin()+1,turtle_stack().end());// =vector<logo_turtle>(1,logo_turtle());
#endif
    history_plot(contextptr).clear();
    while (confirm((lang?"Memoire remplie!":"Memory full"),"Purge variable",true)==-1)
      ;
    gen g=select_var();
    if (g.type==_IDNT)
      _purge(g,contextptr);
    else 
      _restart(0,contextptr);
    SetQuitHandler(0);
    confirm(lang?"Sauvegarde automatique désactivée":"Auto-save disabled",lang?"":"");
  }
  //Console_Output("Done"); return ;
  esc_flag=0;
  ctrl_c=false;
  kbd_interrupted=interrupted=false;
}

void displaylogo(){
#ifdef CASCAS_ALEVEL_ONLY
  return;
#else
#ifdef TURTLETAB
  xcas::Turtle t={tablogo,0,0,1,1};
#else
  xcas::Turtle t={&turtle_stack(),0,0,1,1};
#endif
  int save_ymin=clip_ymin;
  clip_ymin=24;
  t.draw();
  clip_ymin=save_ymin;
  DefineStatusMessage((char*)"+-: zoom, pad: move, EXIT: quit", 1, 0, 0);
  while (1){
    save_ymin=clip_ymin;
    clip_ymin=24;
    t.draw();
    clip_ymin=save_ymin;
    int key;
    DisplayStatusArea();
    ck_getkey(&key);
    if (key==KEY_CTRL_EXIT || key==KEY_PRGM_ACON || key==KEY_CTRL_MENU || key==KEY_CTRL_EXE || key==KEY_CTRL_VARS)
      break;
    if (key==KEY_CTRL_UP){ t.turtley += 10; }
    if (key==KEY_CTRL_PAGEUP) { t.turtley += 100; }
    if (key==KEY_CTRL_DOWN) { t.turtley -= 10; }
    if (key==KEY_CTRL_PAGEDOWN) { t.turtley -= 100;}
    if (key==KEY_CTRL_LEFT) { t.turtlex -= 10; }
    if (key==KEY_SHIFT_LEFT) { t.turtlex -= 100; }
    if (key==KEY_CTRL_RIGHT) { t.turtlex += 10; }
    if (key==KEY_SHIFT_RIGHT) { t.turtlex += 100;}
    if (key==KEY_CHAR_PLUS) { t.turtlezoom *= 2;}
    if (key==KEY_CHAR_MINUS){ t.turtlezoom /= 2;  }
  }  
#endif
}

bool stringtodouble(const string & s1,double & d){
  gen g(s1,contextptr);
  g=evalf_double(g,1,contextptr);
  if (g.type!=_DOUBLE_){
    confirm("Invalid value",s1.c_str());
    return false;
  }
  d=g._DOUBLE_val;
  return true;
}

bool eqws(char * s,bool eval){ // s buffer must be at least GEN_PRINT_BUFSIZE char
  gen g,ge;
  int dconsole_save=dconsole_mode;
  int ss=strlen(s);
  for (int i=0;i<ss;++i){
    if (s[i]==char(0x9c))
      s[i]='\n';
  }
  if (ss>=2 && (s[0]=='#' || s[0]=='"' ||
		(s[0]=='/' && (s[1]=='/' || s[1]=='*'))
		))
    return textedit(s);
  dconsole_mode=0;
  if (eval)
    do_run(s,g,ge);
  else {
    if (s[0]==0)
      ge=0;
    else
      ge=gen(s,contextptr);
  }
  dconsole_mode=dconsole_save;
  if (is_undef(ge))
    return textedit(s);
  if (ge.type==giac::_SYMB || (ge.type==giac::_VECT && !ge._VECTptr->empty() && !is_numericv(*ge._VECTptr)) ){
    if (islogo(ge)){
      displaylogo();
      return false;
    }
    if (ispnt(ge)){
      xcas::displaygraph(ge,g,contextptr);
      // aborttimer = Timer_Install(0, check_execution_abort, 100); if (aborttimer > 0) { Timer_Start(aborttimer); }
      return false;
    }
    if (ge.is_symb_of_sommet(at_program))
      return textedit(s);
    if (taille(ge,xcas::max_prettyprint_equation)>=xcas::max_prettyprint_equation)
      return false; // sizeof(eqwdata)=48
  }
  gen tmp=eqw(ge,true);
  if (is_undef(tmp) || tmp==ge || taille(ge,256)>=256)
    return false;
  string S(tmp.print(contextptr));
  if (S.size()>=GEN_PRINT_BUFSIZE)
    return false;
  strcpy(s,S.c_str());
  return true;
}

void check_do_graph(giac::gen & ge,const giac::gen & gs,int do_logo_graph_eqw,GIAC_CONTEXT) {
  if (ge.type==giac::_SYMB || (ge.type==giac::_VECT && !ge._VECTptr->empty() && !is_numericv(*ge._VECTptr)) ){
    if (islogo(ge)){
      if (do_logo_graph_eqw & 4)
	displaylogo();
      return;
    }
    if (ispnt(ge)){
      if (do_logo_graph_eqw & 2){
	xcas::displaygraph(ge,gs,contextptr);
      }        
      // aborttimer = Timer_Install(0, check_execution_abort, 100); if (aborttimer > 0) { Timer_Start(aborttimer); }
      return ;
    }
    if ( do_logo_graph_eqw % 2 ==0)
      return;
    if (taille(ge,xcas::max_prettyprint_equation)>=xcas::max_prettyprint_equation || ge.is_symb_of_sommet(at_program))
      return ; // sizeof(eqwdata)=44
    gen tmp=eqw(ge,false);
    if (!is_undef(tmp) && tmp!=ge){
      //dConsolePutChar(147);
      Console_Output((const unsigned char *) ge.print(contextptr).c_str());
      Console_NewLine(LINE_TYPE_INPUT, 1);
      ge=tmp;
    }
  }
}

char * c_load_script(const char * filename){
  if (inexammode)
    return 0;
  if (filename && filename[0]!='\\'){
    // print_msg12("Adding \\\\fls0\\",filename);int key; ck_getkey(&key);
    char file[strlen(filename)+16]="\\\\fls0\\";
    strcat(file,filename);
    return c_load_script(file);
  }
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile,(const unsigned char *) filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  // Check for file existence
  if(hFile < 0) 
    return 0;
  // Returned no error, file exists, open it
  int size = Bfile_GetFileSize_OS(hFile);
  // File exists and has size 'size'
  // Read file into a buffer
  if ((unsigned int)size > MAX_TEXTVIEWER_FILESIZE) {
    Bfile_CloseFile_OS(hFile);
    puts("Stop: script too big");
    return 0; //file too big, return
  }
  unsigned char* asrc = (unsigned char*)malloc(size*sizeof(unsigned char)+5); // 5 more bytes to make sure it fits...
  memset(asrc, 0, size+5); //alloca does not clear the allocated space. Make sure the string is null-terminated this way.
  int rsize = Bfile_ReadFile_OS(hFile, asrc, size, 0);
  Bfile_CloseFile_OS(hFile); //we got file contents, close it
  asrc[rsize]='\0';
  return (char *) asrc;
}

int load_script(const char * filename,ustl::string & s){
  char * asrc=c_load_script(filename);
  if (!asrc)
    return 0;
  s=ustl::string((char *)asrc);
  free(asrc);
  return 1;
}

int run_script(const char* filename) {
  // returns 1 if script was run, 0 otherwise
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0) 
    return 0;
  // Check for file existence
  if(hFile >= 0) // Check if it opened
  {
    // Returned no error, file exists, open it
    int size = Bfile_GetFileSize_OS(hFile);
    // File exists and has size 'size'
    // Read file into a buffer
    if ((unsigned int)size > MAX_TEXTVIEWER_FILESIZE) {
      Bfile_CloseFile_OS(hFile);
      puts("Stop: script too big");
      return 0; //file too big, return
    }
    unsigned char* asrc = (unsigned char*)alloca(size*sizeof(unsigned char)+5); // 5 more bytes to make sure it fits...
    memset(asrc, 0, size+5); //alloca does not clear the allocated space. Make sure the string is null-terminated this way.
    int rsize = Bfile_ReadFile_OS(hFile, asrc, size, 0);
    Bfile_CloseFile_OS(hFile); //we got file contents, close it
    asrc[rsize]='\0';
    execution_in_progress = 1;
    run((char*)asrc);
    execution_in_progress = 0;
    if (asrc[0]=='#' || (asrc[0]=='d' && asrc[1]=='e' && asrc[2]=='f' && asrc[3]==' '))
      return 2;
    if ( (asrc[0]=='/' && asrc[1]=='/') ||
	 (rsize>8 && asrc[0]=='f' && (asrc[1]=='o' || asrc[1]=='u') && asrc[2]=='n' && asrc[3]=='c' && asrc[4]=='t' && asrc[5]=='i' && asrc[6]=='o' && asrc[7]=='n' && asrc[8]==' ')
	 )
      return 3;
    return 1;
  }
  return 0;
}

int get_set_session_setting(int value) {
  // value is -1 to get only
  // 0 to disable sesison save/load, 1 to enable
  // if MCS file is present, disable session load/restore
  if(value == -1) {
    int size;
    MCSGetDlen2(DIRNAME, (unsigned char*)SESSIONFILE, &size);
    if (!size) return 1;
    else return 0;
  } else if(value == 1) {
    MCSDelVar2(DIRNAME, (unsigned char*)SESSIONFILE);
    return 1;
  } else if(value == 0) {
    MCS_CreateDirectory(DIRNAME);
    unsigned char buffer[2];
    buffer[0] = 1;
    buffer[1] = 1;
    MCSPutVar2(DIRNAME, (unsigned char*)SESSIONFILE, 2, buffer);
    return 0;
  }
  return -1;
}

#if 0
void create_data_folder() {
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)DATAFOLDER, strlen(DATAFOLDER)+1);
  Bfile_CreateEntry_OS(pFile, CREATEMODE_FOLDER, 0);
}
#endif


string remove_path0(const string & st){
  int s=int(st.size()),i;
  for (i=s-1;i>=0;--i){
    if (st[i]=='\\')
      break;
  }
  return st.substr(i+1,s-i-1);
}

void save(const char * fname){
  (void) fname;
  console_changed=0;
}

void save_session(){
  console_changed=0;
}

int restore_session(const char * fname){
  (void) fname;
  return 0;
}

int select_script_and_run() {
  char filename[MAX_FILENAME_SIZE+1];
  if(fileBrowser(filename, (char*)"*.py", "Scripts")) 
    return run_script(filename);
  return 0;
}

void erase_script(){
  char filename[MAX_FILENAME_SIZE+1];
  int res=fileBrowser(filename, (char*)"*.py", "Scripts");
  if (res && do_confirm(lang?"Vraiment effacer":"Really erase?")){
    unsigned short pFile[MAX_FILENAME_SIZE+1];
    // create file in data folder (assumes data folder already exists)
    Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1);
    int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
    if (hFile>=0){
      Bfile_CloseFile_OS(hFile);
      Bfile_DeleteEntry(pFile);
    }
  }
}

string extract_name(const char * s){
  int l=strlen(s),i,j;
  for (i=l-1;i>=0;--i){
    if (s[i]=='.')
      break;
  }
  if (i<=0)
    return "f";
  for (j=i-1;j>=0;--j){
    if (s[j]=='\\')
      break;
  }
  if (j<0)
    return "f";
  return string(s+j+1).substr(0,i-j-1);
}

void edit_script(char * fname){
  char fname_[MAX_FILENAME_SIZE+1];
  char * filename=0;
  int res=1;
  if (fname)
    filename=fname;
  else {
    res=fileBrowser(fname_, (char*)"*.py", "Scripts");
    filename=fname_;
  }
  if(res) {
    string s;
    load_script(filename,s);
    if (s.empty()){
      s=python_compat(contextptr)?(lang?"Mode CAS":"CAS mode"):(lang?"Mode CAS":"CAS mode");
      s += " AC F6 12";
      int k=confirm(s.c_str(),"F1: Tortue, F6: Prog",true);
      if (k==-1)
	return;
      if (k==KEY_CTRL_F1)
	s="\nefface;\n ";
      else
	s=python_compat(contextptr)?"def "+extract_name(filename)+"(x):\n  \n  return x":"function "+extract_name(filename)+"(x)\nlocal j;\n  \n  return x;\nffunction";
    }
    // split s at newlines
    if (edptr==0)
      edptr=new textArea;
    if (!edptr) return;
    edptr->elements.clear();
    edptr->clipline=-1;
    edptr->filename=filename;
    edptr->editable=true;
    edptr->changed=false;
    edptr->python=python_compat(contextptr);
    edptr->elements.clear();
    add(edptr,s);
    s.clear();
    edptr->line=0;
    //edptr->line=edptr->elements.size()-1;
    edptr->pos=0;
    int res=doTextArea(edptr);
    if (res==-1)
      python_compat(edptr->python,contextptr);
    dConsolePutChar('\x1e');
  }
}

string khicas_state(){
  giac::gen g(giac::_VARS(-1,contextptr)); 
  int b=xcas_python_eval==1?4:python_compat(contextptr);
  python_compat(0,contextptr);
#if 1
  char buf[8192]="";
  if (g.type==giac::_VECT){
    for (int i=0;i<g._VECTptr->size();++i){
      string s((*g._VECTptr)[i].print(contextptr));
      if (strlen(buf)+s.size()+128<sizeof(buf)){
	strcat(buf,s.c_str());
	strcat(buf,":;");
      }
    }
  }
  python_compat(b,contextptr);
  if (strlen(buf)+128<sizeof(buf)){
    strcat(buf,"compat_removed(");
    strcat(buf,giac::print_INT_(b).c_str());
#ifdef MICROPY_LIB
    strcat(buf,",");
    strcat(buf,giac::print_INT_(pythonjs_heap_size).c_str());
    strcat(buf,",");
    strcat(buf,giac::print_INT_(pythonjs_stack_size).c_str());
#endif
    strcat(buf,");angle_radian(");
    strcat(buf,angle_radian(contextptr)?"1":"0");
    strcat(buf,");with_sqrt(");
    strcat(buf,withsqrt(contextptr)?"1":"0");
    strcat(buf,");");
    if (xcas_sheetptr){
      string s(print_tableur(*xcas_sheetptr,contextptr));
      if (strlen(buf)+s.size()+20<sizeof(buf)){
	strcat(buf,"current_sheet(");
	strcat(buf,s.c_str());
	strcat(buf,");");
      }
    }
  }
  return buf;
#else
  string s(g.print(contextptr));
  python_compat(b,contextptr);
  s += "; compat_removed(";
  s +=  giac::print_INT_(b);
  s += ");angle_radian(";
  s += angle_radian(contextptr)?'1':'0';
  s += ");with_sqrt(";
  s += withsqrt(contextptr)?'1':'0';
  s += ");";
  return s;
#endif
}

void save_khicas_symbols_smem(const char * filename) {
  // save variables in xcas mode,
  // because at load time the parser will be in xcas mode
  string s(khicas_state());
  save_script(filename,s);
}


#if 1
int get_custom_key_handler_state() { return 0; }
int get_custom_fkey_label(int fkey) { return 0; }
#else
int get_custom_key_handler_state() {
  U* tmp = usr_symbol("prizmUIhandleKeys");
  if (!issymbol(tmp)) return 0;
  tmp = get_binding(tmp);
  if(isnonnegativeinteger(tmp)) {
    return !iszero(tmp);
  } else return 0;
}
int get_custom_fkey_label(int fkey) {
  U* tmp;
  if(fkey==0) {
    tmp = usr_symbol("prizmUIfkey1label");
  } else if(fkey==1) {
    tmp = usr_symbol("prizmUIfkey2label");
  } else if(fkey==2) {
    tmp = usr_symbol("prizmUIfkey3label");
  } else if (fkey==3) {
    tmp = usr_symbol("prizmUIfkey4label");
  } else if (fkey==5) {
    tmp = usr_symbol("prizmUIfkey6label");
  } else return 0;
  if (issymbol(tmp)) {
    tmp = get_binding(tmp);
    if(isnonnegativeinteger(tmp)) {
      return *tmp->u.q.a;
    }
  }
  return 0;
}
#endif

void printchar(int c){
  dConsolePutChar(c);
}

void printchar_nowrap(int c){
  printchar(c);
}

void printstr(char *s){
  while (*s)
    printchar(*s++);
}

ustl::string last_ans(){
  const vecteur & v=history_out(contextptr);
  if (!v.empty())
    return v.back().print(contextptr);
  return "";
}

void run(const char * s,int do_logo_graph_eqw){
  if (strlen(s)>=2 && (s[0]=='#' ||
		       (s[0]=='/' && (s[1]=='/' || s[1]=='*'))
		       ))
    return;
  if (cascas_reject_removed_feature(s)){
    Console_Output((const unsigned char*)cascas_pure_method_fallback());
    return;
  }
  cascas::working_string direct;
  if (cascas::eval_with_working(s,direct)){
    Console_Output((const unsigned char*)direct.c_str());
    return;
  }
  if (strcmp(s,"caseval(\"\")")==0 || strcmp(s,"eval_expr(\"\")")==0 || (strlen(s)>=4 && strlen(s)<6 && strncmp(s,"xcas",4)==0)){
    int p=python_compat(contextptr);
    xcas_python_eval=0;
    python_compat(p>0?p&3:0,contextptr);
    if (edptr)
      edptr->python=p>0?p&3:0;
#ifdef MICROPY_LIB
    if (p==4 && ((int) python_heap)>1 && do_confirm((lang==1)?"Effacer le tas programme?":"Clear program heap?"))
      python_free();
#endif
#ifdef QUICKJS
    if (0 && p==-1 && do_confirm((lang==1)?"Effacer le tas QuickJS?":"Clear QuickJS heap?"))
      js_end(global_js_context);
#endif
    *logptr(contextptr) << "CAS mode\n";
    Console_FMenu_Init();
    return ;
  }
  gen g,ge;
#ifdef QUICKJS
  if (strlen(s)>=2 && strlen(s)<4 && strncmp(s,"js",2)==0){
    switch_to_js(contextptr);
    return ;
  }
  if (xcas_python_eval==-1){
    string S(s); 
    if (S.size() && S[0]=='@')
      S=S.substr(1,S.size()-1);
    else
      S="\"use math\";"+S;
    S+='\n';
    char * js=js_ck_eval(S.c_str(),&global_js_context);
    if (js){
      S=js;
      free(js);
      process_freeze();
      update_js_vars();
    }
    else S="Error";
    Console_Output(S.c_str());
    return ;
  }
#endif
#ifdef MICROPY_LIB
  if (strlen(s)>=6 && strlen(s)<8 && strncmp(s,"python",6)==0){
    xcas::switch_to_micropy(true,contextptr);
    //Console_FMenu_Init();    
    return ;
  }
  if (xcas_python_eval==1){
    giac::freeze=freezeturtle=false;
    micropy_ck_eval(s);
    // int key; ck_getkey(&key);
  }
  else 
    do_run(s,g,ge);
#else
  do_run(s,g,ge);
#endif
  process_freeze();
#ifdef MICROPY_LIB
  if (xcas_python_eval==1)
    return ;
#endif
#ifdef QUICKJS
  if (xcas_python_eval==-1)
    return ;
#endif
  // process_freeze();
  if (is_undef(ge)){
    Console_Output((const unsigned char*)cascas_general_method_fallback());
    return;
  }
  int t=giac::taille(g,GIAC_HISTORY_MAX_TAILLE);  
  int te=giac::taille(ge,GIAC_HISTORY_MAX_TAILLE);
  bool do_tex=false;
  if (t<GIAC_HISTORY_MAX_TAILLE && te<GIAC_HISTORY_MAX_TAILLE){
    giac::vecteur &vin=history_in(contextptr);
    giac::vecteur &vout=history_out(contextptr);
    if (vin.size()>GIAC_HISTORY_SIZE)
      vin.erase(vin.begin());
    vin.push_back(g);
    if (vout.size()>GIAC_HISTORY_SIZE)
      vout.erase(vout.begin());
    vout.push_back(ge);
  }
  check_do_graph(ge,g,do_logo_graph_eqw,contextptr);
  string s_;
  if (ge.type==giac::_STRNG)
    s_='"'+*ge._STRNGptr+'"';
  else {
    te=giac::taille(ge,256);
    if (te>=256)
      s_="Object too large";
    else {
      if (ge.is_symb_of_sommet(giac::at_pnt) || (ge.type==giac::_VECT && !ge._VECTptr->empty() && ge._VECTptr->back().is_symb_of_sommet(giac::at_pnt)))
	s_="Object";
      else {
	//do_tex=ge.type==giac::_SYMB && has_op(ge,*giac::at_inv);
	// tex support has been disabled!
	s_=ge.print(contextptr);
	// translate to tex? set do_tex to true
      }
    }
  }
  if (s_.size()>=GEN_PRINT_BUFSIZE)
    s_=s_.substr(0,GEN_PRINT_BUFSIZE-4)+"...";
  if (s_.size()>=EDIT_LINE_MAX)
    s_=s_.substr(0,EDIT_LINE_MAX-4)+"...";
  if (cascas_error_like_output(s_))
    s_=cascas_general_method_fallback();
  char* edit_line = (char*)Console_GetEditLine();
  Console_Output((const unsigned char*)s_.c_str());
  //return ge; 
}

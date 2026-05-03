#include "kdisplay.h" 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include "catalogGUI.hpp" // for lang
#include "menuGUI.hpp"
#include "console.h"
#include "textGUI.hpp"
#include "main.h"
#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include "graphicsProvider.hpp"
#include "fileGUI.hpp"
extern giac::context * contextptr;
extern "C" int inexammode;

inline int os_draw_string_small_(int x,int y,const char * s){ return os_draw_string_small(x,y,FL_BLACK,FL_WHITE,s,false);}

inline int os_draw_string_(int x,int y,const char * s){ return os_draw_string(x,y,0,65535,s,false);}

xcas::tableur * xcas_sheetptr=0;
#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC
  xcas::tableur * new_tableur(GIAC_CONTEXT){
    xcas::tableur * sheetptr=new xcas::tableur;
#ifdef NUMWORKS
    sheetptr->nrows=14; sheetptr->ncols=4;
#else
    sheetptr->nrows=20; sheetptr->ncols=5;
#endif
    gen g=vecteur(sheetptr->ncols);
    sheetptr->m=makefreematrice(vecteur(sheetptr->nrows,g));
    makespreadsheetmatrice(sheetptr->m,contextptr);
    sheetptr->cur_row=sheetptr->cur_col=sheetptr->disp_row_begin=sheetptr->disp_col_begin=0;
    sheetptr->sel_row_begin=sheetptr->sel_col_begin=-1;
    sheetptr->cmd_pos=sheetptr->cmd_row=sheetptr->cmd_col=sheetptr->disp_start=-1;
    sheetptr->changed=false;
    sheetptr->recompute=true;
    sheetptr->matrix_fill_cells=true;
    sheetptr->movedown=true;
    sheetptr->filename="session";
    return sheetptr;
  }
  gen current_sheet(const gen & g,GIAC_CONTEXT){
    if (!xcas_sheetptr)
      xcas_sheetptr=new_tableur(contextptr);
    xcas::tableur & t=*xcas_sheetptr;
    if (ckmatrix(g,true)){
      t.m=*g._VECTptr;
      makespreadsheetmatrice(t.m,contextptr);
      t.cur_row=t.cur_col=0;
      t.nrows=t.m.size();
      t.ncols=t.m.front()._VECTptr->size();
      t.sel_row_begin=-1;
      t.cmd_row=t.cmd_pos=-1;
      return 1;
    }
    int r,c;
    if (iscell(g,c,r,contextptr)){
      if (r>=t.nrows||c>=t.ncols)
	return undef;
      gen tmp=t.m[r];
      tmp=tmp[c];
      return tmp[1];
    }
    if (g.type==_VECT && g.subtype==0 && g._VECTptr->empty())
      return gen(extractmatricefromsheet(t.m,false),_SPREAD__VECT);
    gen m(extractmatricefromsheet(t.m),_MATRIX__VECT);
    if (g.type==_VECT && g._VECTptr->empty())
      return m;
    return m[g];
  }
  static const char _current_sheet_s []="current_sheet";
  static define_unary_function_eval(__current_sheet,&current_sheet,_current_sheet_s);
  define_unary_function_ptr5( at_current_sheet ,alias_at_current_sheet,&__current_sheet,_QUOTE_ARGUMENTS,true);
  
#ifndef NO_NAMESPACE_GIAC
}
#endif // ndef NO_NAMESPACE_GIAC


using namespace std;
using namespace giac;
using namespace xcas;

#if 0
int ext_main(){
  while (1){
    statuslinemsg("Numworks loader");
    drawRectangle(0,0,LCD_WITH_PX,LCD_HEIGHT_PX,_BLACK);
    os_draw_string(0,20,_WHITE,_BLACK,"1. Khicas shell");
    os_draw_string(0,40,_WHITE,_BLACK,"2. Epsilon (Numworks HOME)");
    int k=getkey(1);
    if (k=='1' ) run_epsilon();
    if (k=='2') caseval("*");
  }
}
#else
int ext_main(){
  caseval("*");
  return 0;
}
#endif

void handle_flash(GIAC_CONTEXT);

unsigned short mmind_col[]={COLOR_BLUE,COLOR_RED,COLOR_MAGENTA,COLOR_GREEN,COLOR_CYAN,COLOR_YELLOW};

void mastermind_disp(const vector<int> & solution,const vector< vector<int> > & essais,const vector<int> & essai,bool fulldisp,GIAC_CONTEXT){
  int x0=30,y0=30;
  if (fulldisp)
    drawRectangle(0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX,_WHITE);
  else
    drawRectangle(0,y0+6*20,LCD_WIDTH_PX,LCD_HEIGHT_PX-(y0+4*20),_WHITE);
  if (fulldisp){
    // grille
    for (int i=y0;i<=y0+4*20;i+=20)
      draw_line(x0,i,x0+12*20,i,_BLACK);
    for (int j=x0;j<=x0+12*20;j+=20)
      draw_line(j,y0,j,y0+4*20,_BLACK);
    // affichage des coups precedents et resultats
    for (int c=0;c<essais.size();++c){
      const vector<int> & essai=essais[c];
      for (int i=0;i<4;++i){
	draw_filled_circle(x0+20*c+10,y0+20*i+10,10,mmind_col[essai[i]],true,true,contextptr);
      }
      // resultats
      vector<int> S(solution),E(essai);
      // bien places
      int bien=0;
      for (int i=0;i<S.size();++i){
	if (S[i]==E[i]){
	  ++bien;
	  S.erase(S.begin()+i);
	  E.erase(E.begin()+i);
	  --i;
	}
      }
      // mal places
      int mal=0;
      sort(S.begin(),S.end());
      sort(E.begin(),E.end());
      int s=0,e=0;
      for (;;){
	if (s>=S.size() || e>=E.size())
	  break;
	if (S[s]==E[e]){
	  ++mal;
	  ++s; ++e;
	  continue;
	}
	if (S[s]<E[e])
	  ++s;
	else
	  ++e;
      }
      char buf[2]={0,0};
      buf[0]='0'+bien;
      os_draw_string(x0+20*c+3,y0+20*4+2,COLOR_GREEN,_WHITE,buf);
      buf[0]='0'+mal;
      os_draw_string(x0+20*c+3,y0+20*5+2,COLOR_MAGENTA,_WHITE,buf);
      //CERR << solution << " " << essai << " " << bien << " " << mal << endl;
    }
  }
  int y=170,y1=y;
  int x=os_draw_string_small_(x0,y1,"0");
  draw_filled_circle(x+10,y+10,10,COLOR_BLUE);
  x=os_draw_string_small_(x+30,y1,"1");
  draw_filled_circle(x+10,y+10,10,COLOR_RED);
  x=os_draw_string_small_(x+30,y1,"2");
  draw_filled_circle(x+10,y+10,10,COLOR_MAGENTA);
  x=os_draw_string_small_(x+30,y1,"3");
  draw_filled_circle(x+10,y+10,10,COLOR_GREEN);
  x=os_draw_string_small_(x+30,y1,"4");
  draw_filled_circle(x+10,y+10,10,COLOR_CYAN);
  x=os_draw_string_small_(x+30,y1,"5");
  draw_filled_circle(x+10,y+10,10,COLOR_YELLOW);
  y += 20; 
  // affichage du coup actuel
  for (int i=0;i<essai.size();++i)
    draw_filled_circle(x0+20*i+10,y+10,10,mmind_col[essai[i]],true,true,contextptr);
}    
  
int mastermind(GIAC_CONTEXT){
  // Mastermind
  vector<int> solution(4),essai;
  vector< vector<int> > essais;
  const int nbcouleurs=6;
  const int nbessais=12;
  for (int i=0;i<4;++i)
    solution[i]=giac_rand(contextptr) % nbcouleurs;
  int i=0,j=0;
  bool fulldisp=true;
  for (;;){
    mastermind_disp(solution,essais,essai,fulldisp,contextptr);
    // saisie du prochain coup
    int key; ck_getkey(&key);
    if (key==KEY_SHUTDOWN)
      return key;
    fulldisp=false;
    if (key==KEY_CTRL_EXIT)
      return key;
    if (key==KEY_PRGM_ACON){
      fulldisp=true;
      continue;
    }
    if (key>='0' && key<='5'){
      if (essai.size()==4)
	continue;
      essai.push_back(key-'0');
    }
    if (key==KEY_CTRL_EXE || key==KEY_CTRL_OK){
      if (essai.size()==4){
	if (essai==solution){
	  char buf[16]; sprint_int(buf,essais.size());
	  confirm(lang==1?"Vous avez trouve. Essais:":"Congratulations. Trials:",buf);
	  return i;
	}
	fulldisp=true;
	essais.push_back(essai);
	essai.clear();
	if (essais.size()==nbessais){
	  mastermind_disp(solution,essais,essai,true,contextptr);
	  for (int i=0;i<solution.size();++i)
	    draw_filled_circle(30+20*i+10,190+20,10,mmind_col[solution[i]],true,true,contextptr);
	  confirm4(lang==1?"Vous avez perdu.":"Game over.",lang==1?"La solution etait":"Solution was",false,140);
	  return -1;
	}
      }
    }
    if (key==KEY_CTRL_DEL){
      if (!essai.empty())
	essai.pop_back();
      continue;
    }
  }
  return 0;
}

#if 1
// Newton iteration for polynomial
// with simult Horner evaluation of p and p' at x
complex<double> horner_newton(const vector<std::complex<double>> & p,const std::complex<double> &x){
  complex<double> num,den;
  vector<std::complex<double>>::const_iterator it=p.begin(),itend=p.end();
  int n=itend-it-1; 
  for (;n;--n,++it){
    num *= x;
    den *= x;
    num += *it;
    den += double(n)*(*it);
  } // end for
  // last step
  num *= x;
  num += *it;
  return x-num/den;
}

complex<double> horner_newton(const vector<double> & p,const std::complex<double> &x){
  vector<double>::const_iterator it=p.begin(),itend=p.end();
  int n=itend-it-1; 
  complex<double> num=*it*x+*(it+1),den=(double(n)*(*it))*x+double(n-1)*(*(it+1));
  for (it+=2,n-=2;n;--n,++it){
    num *= x;
    den *= x;
    num += *it;
    den += double(n)*(*it);
  } // end for
  // last step
  num *= x;
  num += *it;
  return x-num/den;
}

int fractale(GIAC_CONTEXT){
  freeze=true;
  int X=384,Y=216-24,Nmax=16,Nmaxmin=5,Nmaxmax=50;
  bool mandel=do_confirm("EXE: Mandelbrot, EXIT: Newton");
  vecteur P; vector<complex<double>> p,Z;
  double np=0; complex<double> na;
  // if the polynomial is x^np+a=0
  // Newton iteration is x-(x^n+a)/(n*x^(n-1))=((n-1)*x-a)/(n*x^(n-1))
  vector<double> pr;
  bool real=true;
  if (!mandel){ // Input Julia
    string s;
    inputline("Polynome (x^3-1)?","",s,false,65);
    if (s.empty()) s="x^3-1";
    gen g(s,contextptr);
    g=_symb2poly(eval(g,1,contextptr),contextptr);
    if (g.type!=_VECT || g._VECTptr->size()<3 || g._VECTptr->size()>9){
      confirm("Not a polynomial or deg<2 or deg>8",g.print());
      return 0;
    }
    P=*g._VECTptr;
    if (!convert(P,p,true)){
      do_confirm("Unable to convert");
      return 0;
    }
    // detect x^n+a==0
    np=P.size()-1;
    for (int i=1;i<p.size()-1;++i){
      if (p[i]!=0){
	np=0;
	break;
      }
    }
    if (np){
      na=p.back()/p.front()/double(np);
      np=(np-1)/np;
    }
    for (int i=0;i<p.size();++i){
      if (p[i].imag()!=0){
	real=false;
	break;
      }
      pr.push_back(p[i].real());
    }
    gen R=_proot(P,contextptr);
    if (R.type==_VECT && !convert(*R._VECTptr,Z,true)){
      do_confirm("Unable to find polynomial roots");
      return 0;
    }
  }
  double xmin=-2.1,xmax=1.64,ymin=-0.935,ymax=0.935;
  if (!mandel){
    xmin=-1.87; xmax=1.87; 
  }
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
  while (1){
    os_fill_rect(0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX,COLOR_BLACK);
    statuslinemsg("DEL: interrupt & quit");
    double w=(xmax-xmin)/X;
    double h=(ymin-ymax)/Y;
    bool sym=real && ymin<0 && ymax>0;
    int Ysym=2*ymax/(ymax-ymin)*Y-1;
    for (int y=0;y<Y;++y){
      if (!is_emulator){
	int key,ret=in_ckgetkey(&key,false,0,0,0,0);
	if (ret>=0){
	  if (key==KEY_CTRL_DEL)
	    return true;
	}
      }
      int ysym=Ysym-y; // symmetric pixel
      if (mandel){
	complex<double> c(xmin,h*y+ymax);
	for (int x=0;x<X;++x){
	  int j=0;
	  complex<double> z(0);
	  for (j=0;j<Nmax;++j){
	    z*=z; z+=c;
	    if (norm(z)>4) // this is more efficient than abs(z)>2
	      break;
	  }
	  int color=126*j+2079;
	  os_set_pixel(x,24+y,color);
	  if (sym && ysym>0 && ysym<Y){
	    os_set_pixel(x,24+ysym,color);
	  }
	  c = c+w;
	}
      }
      else {
	complex<double> c(xmin,h*y+ymax);
	for (int x=0;x<X;++x){
	  complex<double> z(c),zp;
	  int nrac=Z.size(),j;
	  // Newton iterations
	  for (j=0;j<Nmax;++j){
	    if (norm(z)>1e20)
	      break;
	    zp=z;
	    if (np){
	      z *=z ;
	      for (int i=3;i<P.size()-1;++i)
		z *= zp;
	      z=np*zp-na/z;
	    }
	    else
	      z=real?horner_newton(pr,zp):horner_newton(p,zp);
	    if (norm(z-zp)<1e-8){
	      // find nearest root
	      for (int i=0;i<nrac;++i){
		if (norm(z-Z[i])<1e-8){
		  nrac=i;
		  break;
		}
	      }
	      break;
	    }
	  }
	  int color=0;
	  if (nrac<Z.size()){
	    int r_,g_,b_; arc_en_ciel(25*nrac+j,r_,g_,b_);
	    color=(((r_*32)/256)<<11) | (((g_*64)/256)<<5) | (b_*32/256);
	  }
	  os_set_pixel(x,24+y,color);	
	  if (sym && ysym>0 && ysym<Y){
	    if (nrac<Z.size()){
	      z=conj(Z[nrac]);
	      // find nearest root
	      for (int i=0;i<Z.size();++i){
		if (norm(z-Z[i])<1e-8){
		  nrac=i;
		  break;
		}
	      }
	      int r_,g_,b_; arc_en_ciel(25*nrac+j,r_,g_,b_);
	      color=(((r_*32)/256)<<11) | (((g_*64)/256)<<5) | (b_*32/256);
	    }
	    os_set_pixel(x,24+ysym,color);
	  }
	  c = c+double(w);	  
	}
      }
      if (sym && (ysym==y || ysym==y-1))
	y=2*y-1;
      if (y%16==0) sync_screen();
    }
    lkey: 
    statuslinemsg("EXIT: quit, +-: zoom, keypad: move, ml: iter");
    int k; ck_getkey(&k);
    if (k==KEY_CTRL_EXIT)
      break;
    double dx=xmax-xmin,dy=ymax-ymin;
    if (k==KEY_CTRL_LEFT){
      xmin -= dx/10;
      xmax -= dx/10;
      continue;
    }
    if (k==KEY_CTRL_RIGHT){
      xmin += dx/10;
      xmax += dx/10;
      continue;
    }
    if (k==KEY_CTRL_DOWN){
      ymin -= dy/10;
      ymax -= dy/10;
      continue;
    }
    if (k==KEY_CTRL_UP){
      ymin += dy/10;
      ymax += dy/10;
      continue;
    }
    double xc=(xmin+xmax)/2,yc=(ymin+ymax)/2;
    if (k==KEY_CHAR_MINUS){
      dx *=1.5; dy*=1.5;
      xmin = xc-dx/2;
      xmax = xc+dx/2;
      ymin = yc-dy/2;
      ymax = yc+dy/2;
      continue;
    }
    if (k==KEY_CHAR_PLUS){
      dx /=1.5; dy/=1.5;
      xmin = xc-dx/2;
      xmax = xc+dx/2;
      ymin = yc-dy/2;
      ymax = yc+dy/2;
      continue;
    }
    if ( (k=='l' || k=='<' || k==KEY_CHAR_ROOT) && Nmax>Nmaxmin){
      --Nmax; continue;
    }
    if ( (k=='m' || k=='>' || k=='7' || k==KEY_CHAR_SQUARE) && Nmax<Nmaxmax){
      ++Nmax; continue;
    }
    goto lkey;
  }
  return 0;
}
#else
int fractale(GIAC_CONTEXT){
  freeze=true;
  int X=384,Y=216-24,Nmax=10;
  double d=16;
  if (inputdouble(lang==1?"Nombre d'iterations? (defaut 16)":"Number of iterations? (default 16)",d,contextptr) && d>=1 && d<=20)
    Nmax=d;
  drawRectangle(0,24,LCD_WIDTH_PX,LCD_HEIGHT_PX-24,_BLACK); 
  float w=3.74/X; // 2.7/X;
  float h=-1.87/Y;
  for (int y=0;y<=Y/2;++y){
    float cre=-2.1,cim=h*y+0.935;
    for (int x=0;x<X;++x){
      float zre=0,zim=0;
      int j;
      for (j=0;j<Nmax;++j){
	float re=zre*zre-zim*zim+cre;
	zim=2*zre*zim+cim;
	zre=re;
	if (zre*zre+zim*zim>4)
	  break;
      }
      int color=126*j+2079;
      os_set_pixel(x,24+y,color);
      os_set_pixel(x,24+(Y-y),color);
      cre += w;
    }
    if (y%16==0) sync_screen();
  }
  statuslinemsg("Ecran fige. Taper EXIT");
  int key;ck_getkey(&key);
  return 0;
}
#endif

  int rgb24to16(int c){
    int r=(c>>16)&0xff,g=(c>>8)&0xff,b=c&0xff;
    return (((r*32)/256)<<11) | (((g*64)/256)<<5) | (b*32/256);
  }

  void stroke_rectangle(int x,int y,int w,int h,int c){
    drawLine(x,y,x+w,y,c);
    drawLine(x,y+h,x+w,y+h,c);
    drawLine(x,y,x,y+h,c);
    drawLine(x+w,y,x+w,y+h,c);
  }
  // table periodique, code adapte de https://github.com/M4xi1m3/nw-atom
  // avec l'aimable autorisation de diffusion sous licence GPL de Maxime Friess
  // https://tiplanet.org/forum/viewtopic.php?f=97&t=23094&p=247471#p247471
enum AtomType {
  ALKALI_METAL,
  ALKALI_EARTH_METAL,
  LANTHANIDE,
  ACTINIDE,
  TRANSITION_METAL,
  POST_TRANSITION_METAL,
  METALLOID,
  HALOGEN,
  REACTIVE_NONMETAL,
  NOBLE_GAS,
  UNKNOWN
};

struct AtomDef {
  uint8_t num;
  uint8_t x;
  uint8_t y;
  AtomType type;
  const char* name;
  const char* symbol;
  uint8_t neutrons;
  double mass;
  double electroneg;
};

const AtomDef atomsdefs[] = {
  {  1,  0,  0, REACTIVE_NONMETAL       , "Hydrogen"     , "H"   ,   0, 1.00784     , 2.2   },
  {  2, 17,  0, NOBLE_GAS               , "Helium"       , "He"  ,   2, 4.002602    , -1    },
  
  
  {  3,  0,  1, ALKALI_METAL            , "Lithium"      , "Li"  ,   4, 6.938       , 0.98  },
  {  4,  1,  1, ALKALI_EARTH_METAL      , "Beryllium"    , "Be"  ,   5, 9.012182    , 1.57  },
  {  5, 12,  1, METALLOID               , "Boron"        , "B"   ,   6, 10.806      , 2.04  },
  {  6, 13,  1, REACTIVE_NONMETAL       , "Carbon"       , "C"   ,   6, 12.0096     , 2.55  },
  {  7, 14,  1, REACTIVE_NONMETAL       , "Nitrogen"     , "N"   ,   7, 14.00643    , 3.04  },
  {  8, 15,  1, REACTIVE_NONMETAL       , "Oxygen"       , "O"   ,   8, 15.99903    , 3.44  },
  {  9, 16,  1, HALOGEN                 , "Fluorine"     , "F"   ,  10, 18.9984032  , 3.98  },
  { 10, 17,  1, NOBLE_GAS               , "Neon"         , "Ne"  ,  10, 20.1797     , -1    },
  
  
  { 11,  0,  2, ALKALI_METAL            , "Sodium"       , "Na"  ,  12, 22.9897693  , 0.93  },
  { 12,  1,  2, ALKALI_EARTH_METAL      , "Magnesium"    , "Mg"  ,  12, 24.3050     , 1.31  },
  { 13, 12,  2, POST_TRANSITION_METAL   , "Aluminium"    , "Al"  ,  14, 26.9815386  , 1.61  },
  { 14, 13,  2, METALLOID               , "Silicon"      , "Si"  ,  14, 28.084      , 1.9   },
  { 15, 14,  2, REACTIVE_NONMETAL       , "Phosphorus"   , "P"   ,  16, 30.973762   , 2.19  },
  { 16, 15,  2, REACTIVE_NONMETAL       , "Sulfur"       , "S"   ,  16, 32.059      , 2.58  },
  { 17, 16,  2, HALOGEN                 , "Chlorine"     , "Cl"  ,  18, 35.446      , 3.16  },
  { 18, 17,  2, NOBLE_GAS               , "Argon"        , "Ar"  ,  22, 39.948      , -1    },
  
  
  { 19,  0,  3, ALKALI_METAL            , "Potassium"    , "K"   ,  20, 39.0983     , 0.82  },
  { 20,  1,  3, ALKALI_EARTH_METAL      , "Calcium"      , "Ca"  ,  20, 40.078      , 1     },
  { 21,  2,  3, TRANSITION_METAL        , "Scandium"     , "Sc"  ,  24, 44.955912   , 1.36  },
  { 22,  3,  3, TRANSITION_METAL        , "Titanium"     , "Ti"  ,  26, 47.867      , 1.54  },
  { 23,  4,  3, TRANSITION_METAL        , "Vanadium"     , "V"   ,  28, 50.9415     , 1.63  },
  { 24,  5,  3, TRANSITION_METAL        , "Chromium"     , "Cr"  ,  28, 51.9961     , 1.66  },
  { 25,  6,  3, TRANSITION_METAL        , "Manganese"    , "Mn"  ,  30, 54.938045   , 1.55  },
  { 26,  7,  3, TRANSITION_METAL        , "Iron"         , "Fe"  ,  30, 55.845      , 1.83  },
  { 27,  8,  3, TRANSITION_METAL        , "Cobalt"       , "Co"  ,  32, 58.933195   , 1.88  },
  { 28,  9,  3, TRANSITION_METAL        , "Nickel"       , "Ni"  ,  30, 58.6934     , 1.91  },
  { 29, 10,  3, TRANSITION_METAL        , "Copper"       , "Cu"  ,  34, 63.546      , 1.9   },
  { 30, 11,  3, POST_TRANSITION_METAL   , "Zinc"         , "Zn"  ,  34, 65.38       , 1.65  },
  { 31, 12,  3, POST_TRANSITION_METAL   , "Gallium"      , "Ga"  ,  38, 69.723      , 1.81  },
  { 32, 13,  3, METALLOID               , "Germanium"    , "Ge"  ,  42, 72.63       , 2.01  },
  { 33, 14,  3, METALLOID               , "Arsenic"      , "As"  ,  42, 74.92160    , 2.18  },
  { 34, 15,  3, REACTIVE_NONMETAL       , "Selenium"     , "Se"  ,  46, 78.96       , 2.55  },
  { 35, 16,  3, HALOGEN                 , "Bromine"      , "Br"  ,  44, 79.904      , 2.96  },
  { 36, 17,  3, NOBLE_GAS               , "Krypton"      , "Kr"  ,  48, 83.798      , -1    },
  
  { 37,  0,  4, ALKALI_METAL            , "Rubidium"     , "Rb"  ,  48, 85.4678     , 0.82  },
  { 38,  1,  4, ALKALI_EARTH_METAL      , "Strontium"    , "Sr"  ,  50, 87.62       , 0.95  },
  { 39,  2,  4, TRANSITION_METAL        , "Yttrium"      , "Y"   ,  50, 88.90585    , 1.22  },
  { 40,  3,  4, TRANSITION_METAL        , "Zirconium"    , "Zr"  ,  50, 91.224      , 1.33  },
  { 41,  4,  4, TRANSITION_METAL        , "Niobium"      , "Nb"  ,  52, 92.90638    , 1.6   },
  { 42,  5,  4, TRANSITION_METAL        , "Molybdenum"   , "Mo"  ,  56, 95.96       , 2.16  },
  { 43,  6,  4, TRANSITION_METAL        , "Technetium"   , "Tc"  ,  55, 98          , 2.10  },
  { 44,  7,  4, TRANSITION_METAL        , "Ruthemium"    , "Ru"  ,  58, 101.07      , 2.2   },
  { 45,  8,  4, TRANSITION_METAL        , "Rhodium"      , "Rh"  ,  58, 102.90550   , 2.28  },
  { 46,  9,  4, TRANSITION_METAL        , "Palladium"    , "Pd"  ,  60, 106.42      , 2.20  },
  { 47, 10,  4, TRANSITION_METAL        , "Silver"       , "Ag"  ,  60, 107.8682    , 1.93  },
  { 48, 11,  4, POST_TRANSITION_METAL   , "Cadmium"      , "Cd"  ,  66, 112.411     , 1.69  },
  { 49, 12,  4, POST_TRANSITION_METAL   , "Indium"       , "In"  ,  66, 114.818     , 1.78  },
  { 50, 13,  4, POST_TRANSITION_METAL   , "Tin"          , "Sn"  ,  70, 118.710     , 1.96  },
  { 51, 14,  4, METALLOID               , "Antimony"     , "Sb"  ,  70, 121.760     , 2.05  },
  { 52, 15,  4, METALLOID               , "Tellurium"    , "Te"  ,  78, 127.60      , 2.1   },
  { 53, 16,  4, HALOGEN                 , "Indine"       , "I"   ,  74, 126.90447   , 2.66  },
  { 54, 17,  4, NOBLE_GAS               , "Xenon"        , "Xe"  ,  78, 131.293     , 2.60  },
  
  
  { 55,  0,  5, ALKALI_METAL            , "Caesium"      , "Cs"  ,  78, 132.905452  , 0.79  },
  { 56,  1,  5, ALKALI_EARTH_METAL      , "Barium"       , "Ba"  ,  82, 137.327     , 0.89  },

  { 57,  3,  7, LANTHANIDE              , "Lanthanum"    , "La"  ,  82, 138.90547   , 1.10  },
  { 58,  4,  7, LANTHANIDE              , "Cerium"       , "Ce"  ,  82, 140.116     , 1.12  },
  { 59,  5,  7, LANTHANIDE              , "Praseodymium" , "Pr"  ,  82, 140.90765   , 1.13  },
  { 60,  6,  7, LANTHANIDE              , "Neodymium"    , "Nd"  ,  82, 144.242     , 1.14  },
  { 61,  7,  7, LANTHANIDE              , "Promethium"   , "Pm"  ,  84, 145         , 1.13  },
  { 62,  8,  7, LANTHANIDE              , "Samarium"     , "Sm"  ,  90, 150.36      , 1.17  },
  { 63,  9,  7, LANTHANIDE              , "Europium"     , "Eu"  ,  90, 151.964     , 1.12  },
  { 64, 10,  7, LANTHANIDE              , "Gadolinium"   , "Gd"  ,  94, 157.25      , 1.20  },
  { 65, 11,  7, LANTHANIDE              , "Terbium"      , "Tb"  ,  94, 158.92535   , 1.12  },
  { 66, 12,  7, LANTHANIDE              , "Dyxprosium"   , "Dy"  ,  98, 162.500     , 1.22  },
  { 67, 13,  7, LANTHANIDE              , "Holmium"      , "Ho"  ,  98, 164.93032   , 1.23  },
  { 68, 14,  7, LANTHANIDE              , "Erbium"       , "Er"  ,  98, 167.259     , 1.24  },
  { 69, 15,  7, LANTHANIDE              , "Thulium"      , "Tm"  , 100, 168.93421   , 1.25  },
  { 70, 16,  7, LANTHANIDE              , "Ytterbium"    , "Yb"  , 104, 173.054     , 1.1   },
  { 71, 17,  7, LANTHANIDE              , "Lutetium"     , "Lu"  , 104, 174.9668    , 1.0   },

  { 72,  3,  5, TRANSITION_METAL        , "Hafnium"      , "Hf"  , 108, 178.49      , 1.3   },
  { 73,  4,  5, TRANSITION_METAL        , "Tantalum"     , "Ta"  , 108, 180.94788   , 1.5   },
  { 74,  5,  5, TRANSITION_METAL        , "Tungsten"     , "W"   , 110, 183.84      , 1.7   },
  { 75,  6,  5, TRANSITION_METAL        , "Rhenium"      , "Re"  , 112, 186.207     , 1.9   },
  { 76,  7,  5, TRANSITION_METAL        , "Osmium"       , "Os"  , 116, 190.23      , 2.2   },
  { 77,  8,  5, TRANSITION_METAL        , "Iridium"      , "Ir"  , 116, 192.217     , 2.2   },
  { 78,  9,  5, TRANSITION_METAL        , "Platinum"     , "Pt"  , 117, 195.084     , 2.2   },
  { 79, 10,  5, TRANSITION_METAL        , "Gold"         , "Au"  , 118, 196.966569  , 2.4   },
  { 80, 11,  5, POST_TRANSITION_METAL   , "Mercury"      , "Hg"  , 122, 200.59      , 1.9   },
  { 81, 12,  5, POST_TRANSITION_METAL   , "Thalium"      , "Tl"  , 124, 204.382     , 1.8   },
  { 82, 13,  5, POST_TRANSITION_METAL   , "Lead"         , "Pb"  , 126, 207.2       , 1.8   },
  { 83, 14,  5, POST_TRANSITION_METAL   , "Bismuth"      , "Bi"  , 126, 208.98040   , 1.9   },
  { 84, 15,  5, POST_TRANSITION_METAL   , "Polonium"     , "Po"  , 126, 209         , 2.0   },
  { 85, 16,  5, HALOGEN                 , "Astatine"     , "At"  , 125, 210         , 2.2   },
  { 86, 17,  5, NOBLE_GAS               , "Radon"        , "Rn"  , 136, 222         , 2.2   },
  
  
  { 87,  0,  6, ALKALI_METAL            , "Francium"     , "Fr"  , 136, 223         , 0.7   },
  { 88,  1,  6, ALKALI_EARTH_METAL      , "Radium"       , "Ra"  , 138, 226         , 0.9   },

  { 89,  3,  8, ACTINIDE                , "Actinium"     , "Ac"  , 138, 227         , 1.1   },
  { 90,  4,  8, ACTINIDE                , "Thorium"      , "Th"  , 142, 232.03806   , 1.3   },
  { 91,  5,  8, ACTINIDE                , "Protactinium" , "Pa"  , 140, 231.03588   , 1.5   },
  { 92,  6,  8, ACTINIDE                , "Uranium"      , "U"   , 146, 238.02891   , 1.38   },
  { 93,  7,  8, ACTINIDE                , "Neptunium"    , "Np"  , 144, 237         , 1.36   },
  { 94,  8,  8, ACTINIDE                , "Plutonium"    , "Pu"  , 150, 244         , 1.28   },
  { 95,  9,  8, ACTINIDE                , "Americium"    , "Am"  , 148, 243         , 1.13  },
  { 96, 10,  8, ACTINIDE                , "Curium"       , "Cm"  , 151, 247         , 1.28  },
  { 97, 11,  8, ACTINIDE                , "Berkellum"    , "Bk"  , 150, 247         , 1.3   },
  { 98, 12,  8, ACTINIDE                , "Californium"  , "Cf"  , 153, 251         , 1.3   },
  { 99, 13,  8, ACTINIDE                , "Einsteinium"  , "Es"  , 153, 252         , 1.3   },
  {100, 14,  8, ACTINIDE                , "Fermium"      , "Fm"  , 157, 257         , 1.3   },
  {101, 15,  8, ACTINIDE                , "Mendelevium"  , "Md"  , 157, 258         , 1.3   },
  {102, 16,  8, ACTINIDE                , "Nobelium"     , "No"  , 157, 259         , 1.3   },
  {103, 17,  8, ACTINIDE                , "Lawrencium"   , "Lr"  , 163, 262         , 1.3   },

  {104,  3,  6, TRANSITION_METAL        , "Rutherfordium", "Rf"  , 163, 261         , -1    },
  {105,  4,  6, TRANSITION_METAL        , "Dubnium"      , "Db"  , 163, 262         , -1    },
  {106,  5,  6, TRANSITION_METAL        , "Seaborgium"   , "Sg"  , 163, 263         , -1    },
  {107,  6,  6, TRANSITION_METAL        , "Bohrium"      , "Bh"  , 163, 264         , -1    },
  {108,  7,  6, TRANSITION_METAL        , "Hassium"      , "Hs"  , 169, 265         , -1    },
  {109,  8,  6, UNKNOWN                 , "Meitnerium"   , "Mt"  , 169, 268         , -1    },
  {110,  9,  6, UNKNOWN                 , "Damstadtium"  , "Ds"  , 171, 281         , -1    },
  {111, 10,  6, UNKNOWN                 , "Roentgenium"  , "Rg"  , 171, 273         , -1    },
  {112, 11,  6, POST_TRANSITION_METAL   , "Coppernicium" , "Cn"  , 173, 277         , -1    },
  {113, 12,  6, UNKNOWN                 , "Nihonium"     , "Nh"  , 173, 283         , -1    },
  {114, 13,  6, UNKNOWN                 , "Flerovium"    , "Fl"  , 175, 285         , -1    },
  {115, 14,  6, UNKNOWN                 , "Moscovium"    , "Mv"  , 174, 287         , -1    },
  {116, 15,  6, UNKNOWN                 , "Livermorium"  , "Lv"  , 177, 289         , -1    },
  {117, 16,  6, UNKNOWN                 , "Tennessine"   , "Ts"  , 177, 294         , -1    },
  {118, 17,  6, NOBLE_GAS               , "Oganesson"    , "Og"  , 176, 293         , -1    },
  
};
  
void drawAtom(uint8_t id) {
  int fill = rgb24to16(0xeeeeee);

  switch(atomsdefs[id].type) {
    case ALKALI_METAL:
      fill = rgb24to16(0xffaa00);
      break;
    case ALKALI_EARTH_METAL:
      fill = rgb24to16(0xf6f200);
      break;
    case LANTHANIDE:
      fill = rgb24to16(0xffaa8b);
      break;
    case ACTINIDE:
      fill = rgb24to16(0xdeaacd);
      break;
    case TRANSITION_METAL:
      fill = rgb24to16(0xde999c);
      break;
    case POST_TRANSITION_METAL:
      fill = rgb24to16(0x9cbaac);
      break;
    case METALLOID:
      fill = rgb24to16(0x52ce8b);
      break;
    case REACTIVE_NONMETAL:
      fill = rgb24to16(0x00ee00);
      break;
    case NOBLE_GAS:
      fill = rgb24to16(0x8baaff);
      break;
    case HALOGEN:
      fill = rgb24to16(0x00debd);
      break;
    default:
      break;
  }

  if (atomsdefs[id].y >= 7) {
    drawRectangle(6 + atomsdefs[id].x * 17, 15 + atomsdefs[id].y * 17, 18, 18, fill);
    stroke_rectangle(6 + atomsdefs[id].x * 17, 15 + atomsdefs[id].y * 17, 18, 18, rgb24to16(0x525552));
    os_draw_string_small(8 + atomsdefs[id].x * 17, 17 + atomsdefs[id].y * 17, _BLACK, fill, atomsdefs[id].symbol);
  } else {
    drawRectangle(6 + atomsdefs[id].x * 17, 6 + atomsdefs[id].y * 17, 18, 18, fill);
    stroke_rectangle(6 + atomsdefs[id].x * 17, 6 + atomsdefs[id].y * 17, 18, 18, rgb24to16(0x525552));
    os_draw_string_small(8 + atomsdefs[id].x * 17, 8 + atomsdefs[id].y * 17, _BLACK, fill, atomsdefs[id].symbol);
  }
}

  int periodic_table(const char * & name,const char * & symbol,char * protons,char * nucleons,char * mass,char * electroneg){
    bool partial_draw=false,redraw=true;
    int cursor_pos=0;
    const int ATOM_NUMS=sizeof(atomsdefs)/sizeof(AtomDef);
    for (;;){
      if (redraw){
	if (partial_draw) {
	  partial_draw = false;
	  drawRectangle(50, 0, 169, 57, _WHITE);
	  drawRectangle(0, 185, LCD_WIDTH_PX, 15, _WHITE);
	} else {
	  drawRectangle(0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX,_WHITE);
	}
#ifdef NSPIRE_NEWLIB
	os_draw_string_small_(0,200,gettext("enter: tout, P:protons, N:nucleons, M:mass, E:khi"));
#else
	os_draw_string_small_(0,200,lang==1?("EXE: tout, P:protons, N:nucleons, M:mass, E:khi"):("EXE: all, P:protons, N:nucleons, M:mass, E:khi"));
#endif
	for(int i = 0; i < ATOM_NUMS; i++) {
	  drawAtom(i);
	}
	if (atomsdefs[cursor_pos].y >= 7) {
	  stroke_rectangle(6 + atomsdefs[cursor_pos].x * 17, 15 + atomsdefs[cursor_pos].y * 17, 18, 18, 0x000000);
	  stroke_rectangle(7 + atomsdefs[cursor_pos].x * 17, 16 + atomsdefs[cursor_pos].y * 17, 16, 16, 0x000000);
	} else {
	  stroke_rectangle(6 + atomsdefs[cursor_pos].x * 17, 6 + atomsdefs[cursor_pos].y * 17, 18, 18, 0x000000);
	  stroke_rectangle(7 + atomsdefs[cursor_pos].x * 17, 7 + atomsdefs[cursor_pos].y * 17, 16, 16, 0x000000);
	}
  
	drawRectangle(48,  99, 2, 61,rgb24to16(0x525552));
	drawRectangle(48, 141, 9,  2, rgb24to16(0x525552));
	drawRectangle(48, 158, 9,  2, rgb24to16(0x525552));

	int prot=atomsdefs[cursor_pos].num;
	sprint_int(protons,prot);
	int nuc=atomsdefs[cursor_pos].neutrons+atomsdefs[cursor_pos].num;
	sprint_int(nucleons,nuc);
	
	symbol=atomsdefs[cursor_pos].symbol;
	os_draw_string_(73,23+10,symbol);
	name=atomsdefs[cursor_pos].name;
	os_draw_string_small_(110,27+10,gettext(name));
	os_draw_string_small_(50,18+10,nucleons);
	os_draw_string_small_(50,31+10,protons);
	strcpy(mass,"M:");
	strcpy(electroneg,"khi:");
	sprint_double(mass+2,atomsdefs[cursor_pos].mass);
	os_draw_string_small_(0,186,mass);
	sprint_double(electroneg+4,atomsdefs[cursor_pos].electroneg);
	os_draw_string_small_(160,186,electroneg);
      }
      redraw=false;
      int key;
      GetKey(&key);
      if (key==KEY_SHUTDOWN)
	return key;
      if (key==KEY_PRGM_ACON)
	redraw=true;
      if (key==KEY_CTRL_EXIT)
	return 0;
      if (key==KEY_CTRL_EXE || key==KEY_CTRL_OK)
	return 1|4|8|16|32;
      if (key=='S' || key==KEY_CHAR_MULT)
	return 2;
      if (key=='P' || key==KEY_CHAR_4)
	return 4;
      if (key=='N' || key==KEY_CHAR_8)
	return 8;
      if (key=='M' || key==KEY_CHAR_7)
	return 16;
      if (key=='E' || key==KEY_CHAR_COS)
	return 32;
      if (key==KEY_CTRL_LEFT){
	if (cursor_pos>0)
	  --cursor_pos;
	redraw=partial_draw=true;
      }
      if (key==KEY_CTRL_RIGHT){
	if (cursor_pos< ATOM_NUMS-1)
	  ++cursor_pos;
	redraw=partial_draw=true;
      }
      if (key==KEY_CTRL_UP){
	uint8_t curr_x = atomsdefs[cursor_pos].x;
	uint8_t curr_y = atomsdefs[cursor_pos].y;
	bool updated = false;
	
	if (curr_y > 0 && curr_y <= 9) {
	  for(uint8_t i = 0; i < ATOM_NUMS; i++) {
	    if (atomsdefs[i].x == curr_x && atomsdefs[i].y == curr_y - 1) {
	      cursor_pos = i;
	      redraw=partial_draw = true;
	    }
	  }
	}
	
      }
      if (key==KEY_CTRL_DOWN){
	uint8_t curr_x = atomsdefs[cursor_pos].x;
	uint8_t curr_y = atomsdefs[cursor_pos].y;
	bool updated = false;
	
	if (curr_y >= 0 && curr_y < 9) {
	  for (uint8_t i = 0; i < ATOM_NUMS; i++) {
	    if (atomsdefs[i].x == curr_x && atomsdefs[i].y == curr_y + 1) {
	      cursor_pos = i;
	      redraw=partial_draw = true;
	      break;
	    }
	  }
	}
      }
    } // end endless for
  } // end periodic_table


int finance(int mode,GIAC_CONTEXT){ // mode==-1 pret, 1 placement
  static double pv=(-mode)*10000;
  static double fv=0;
  static double ir=3; // % annual
  static double irpy=12; // per year
  static double pm=100; // mensualite
  static double nb=10; // nombre d'annuites
  pv=(-mode)*fabs(pv);
  double * tabd[6]={&pv,&fv,&ir,&irpy,&pm,&nb};
  bool solved=false;
  Menu smallmenu;
  smallmenu.numitems=7; 
  // and uncomment first smallmenuitems[app_number].text="Reserved"
  // replace by your application name
  // and add if (smallmenu.selection==app_number-1){ call your code }
  MenuItem smallmenuitems[smallmenu.numitems];      
  smallmenu.items=smallmenuitems;
  smallmenu.height=7;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char *) (mode==-1?"Pret bancaire":"Epargne");
  smallmenu.type = MENUTYPE_NO_NUMBER;
  while(1) {
    drawRectangle(0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX,_WHITE);
    string pvs,fvs,pms,irpys;
    if (mode==-1){
      pvs=((lang==1)?"Pret de ":"Present due ");
      fvs=((lang==1)?"Restera du ":"Future due "); 
      pms=((lang==1)?"Mensualite ":"Payment ");
      irpys=((lang==1)?"Paiements/an ":"Payments/year ");
    } else {
      pvs=((lang==1)?"Actuel ":"Present ");
      fvs=((lang==1)?"Futur ":"Future ");
      pms=((lang==1)?"Montant/mois ":"Payment ");
      irpys=((lang==1)?"Versements/an ":"Payments/year ");
    }
    string irs=((lang==1)?"Taux annuel ":"Annual rate ");
    string nbs=((lang==1)?"Nb d'annees ":"Nb of years ");
    string pvs1=pvs+giac::print_DOUBLE_((-mode)*pv,contextptr),
      fvs1=fvs+giac::print_DOUBLE_((-mode)*fv,contextptr),
      irs1=irs+giac::print_DOUBLE_(ir,contextptr)+"%",
      irpys1=irpys+giac::print_DOUBLE_(irpy,contextptr),
      pms1=pms+giac::print_DOUBLE_(pm,contextptr),
      nbs1=nbs+giac::print_DOUBLE_(nb,contextptr);
    char * tab[6]={(char*)pvs1.c_str(),(char*)fvs1.c_str(), (char*)irs1.c_str(),(char*)irpys1.c_str(), (char*)pms1.c_str(),(char*)nbs1.c_str()};
    for (int i=0;i<6;i++)
      smallmenuitems[i].text = tab[i];
    smallmenuitems[6].text = (char*)((lang==1)?"Quitter ":"Quit ");
    os_draw_string(0,200,solved?giac::_GREEN:giac::_MAGENTA,_WHITE,"F1 solve |  EXE change value  | F6 help");
    int sres = doMenu(&smallmenu);
    if (sres==MENU_RETURN_EXIT)
      break;
    int choix=smallmenu.selection-1;
    if (sres == KEY_CTRL_CATALOG || sres==KEY_BOOK || sres==KEY_CTRL_F6) { // Help
      textArea text;
      text.editable=false;
      text.clipline=-1;
      text.title = (char*)((lang==1)?"Calcul d'un pret":"Finance help");
      text.allowF1=true;
      text.python=python_compat(contextptr);
      std::vector<textElement> & elem=text.elements;
      elem = std::vector<textElement> (2);
      elem[0].s = (lang==1)?"Deplacez le curseur sur une ligne, tapez EXE/OK pour entrer une nouvelle valeur ou tapez sur Ans pour resoudre.":"Move cursor on a line, type EXE/OK to enter a new value or type Ans to solve";
      elem[0].newLine = 0;
      if (mode==-1)
	elem[1].s = (lang==1)?"Par exemple entrez le montant de l'emprunt en 1, 0 en 2, le taux d'interet, le nombre d'annees puis placez le curseur en 5 et tapez F1 ou Ans.":"For example, enter due amount in 1, 0 in 2, interest rate, number of years then move cursor on 5 and type F1 or Ans";
      else
	elem[1].s = (lang==1)?"Pour calculer l'evolution d'un placement, entrer le montant place au debut, le taux d'interet, le nombre d'annees, 0 en 5 (paiement) puis deplacez le curseur en 2 et tapez F1 ou Ans":"For example, enter current value in 0, interest rate, number of years, 0 in 5 (payment) then move cursor in 2 and type F1 or Ans";
      elem[1].newLine = 1;
      sres=doTextArea(&text);
      continue;
    }
    if (sres == KEY_CHAR_ANS || sres==KEY_CTRL_F1){
      if (choix==3)
	continue;
      double t1=std::pow(1+ir/100,1./irpy);
      double t=t1-1;
      double & u0=pv;
      double & un=fv;
      double & r=pm;
      double C=r/t;
      double n=nb*irpy;
      // un=(1+t)^n*(u0-r/t)+r/t
      if (choix==0){ // solve for u0=(1+t)^(-n)*(un-r/t)+r/t
	u0=std::pow(t1,-n)*(un-C)+C;
      }
      if (choix==1){
	un=std::pow(t1,n)*(u0-C)+C;
      }
      if (choix==2){ // solve for T
	giac::gen sol=un-pow(1+vx_var,n,contextptr)*(u0-gen(r)/vx_var)-gen(r)/vx_var;
	// initial guess (un+n*r-u0)/(n*u0)
	sol=giac::_fsolve(makesequence(sol,vx_var,(un+n*r-u0)/n/u0),contextptr);
	if (sol.type==_DOUBLE_){
	  t=sol._DOUBLE_val;
	  ir=100*(std::pow(1+t,irpy)-1);
	}
	else continue;
      }
      if (choix==4){ // solve for r=t*(u0*(t+1)**n-﻿un)/((t+1)**n-1)
	double tmp=std::pow(t+1,n);
	r=t*(u0*tmp-un)/(tmp-1);
      }
      if (choix==5){ // solve for n=(-ln(t*u0-r)+ln(t*﻿un-r))/ln(t+1)
	double n=std::log((t*un-r)/(t*u0-r))/std::log(t+1);
	nb=n/irpy;
      }
      solved=true;
    }
    int keynumber=-1;
    if (sres>=KEY_CHAR_0 && sres<=KEY_CHAR_9) keynumber=sres-KEY_CHAR_0;
    if (sres==KEY_CTRL_EXE || sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_OK || keynumber>=0) {
      if (smallmenu.selection==7 && keynumber==-1) // quit
	break;
      double d=*tabd[choix];
      if (choix<2 && mode==1) d=-d;
      if (keynumber>=0)
	d=keynumber;
      if (!inputdouble(tab[choix],d,contextptr))
	continue;
      if (choix<2 && mode==1) d=-d;
      if (choix==3){
	if (d<1)
	  d=1;
	if (d>365)
	  d=365;
      }
      if (choix==5){
	if (d<=0)
	  d=1;
	if (d>365)
	  d=365;
      }
      *tabd[choix]=d;
      solved=false;
    }
  }
  return 0;
}

int geoapp(GIAC_CONTEXT);

int khicas_addins_menu(GIAC_CONTEXT){
  Menu smallmenu;
  smallmenu.numitems=9; // INCREMENT IF YOU ADD AN APPLICATION
  // and uncomment first smallmenuitems[app_number].text="Reserved"
  // replace by your application name
  // and add if (smallmenu.selection==app_number-1){ call your code }
  MenuItem smallmenuitems[smallmenu.numitems];      
  smallmenu.items=smallmenuitems;
  smallmenu.height=8;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenuitems[0].text = (char*)((lang==1)?"Geometrie":"Geometry");
  smallmenuitems[1].text = (char*)((lang==1)?"Tableur":"Spreadsheet");
  smallmenuitems[2].text = (char*)((lang==1)?"Table periodique":"Periodic table");
  smallmenuitems[3].text = (char*)((lang==1)?"Pret":"Mortgage");
  smallmenuitems[4].text = (char*)((lang==1)?"Epargne":"TVM");
  smallmenuitems[5].text = (char*)((lang==1)?"Exemple simple: Syracuse":"Simple example; Syracuse");
  smallmenuitems[6].text = (char*)((lang==1)?"Exemple de jeu: Mastermind":"Game example: Mastermind");
  smallmenuitems[7].text = (char*)((lang==1)?"Fractales Mandelbrot...":"Mandelbrot fractal");
  // smallmenuitems[5].text = (char*)"Mon application"; // adjust numitem !
  // smallmenuitems[6].text = (char*)"Autre application";
  // smallmenuitems[7].text = (char*)"Encore une autre";
  // smallmenuitems[8].text = (char*)"Une avant-derniere";
  // smallmenuitems[9].text = (char*)"Une derniere";
  smallmenuitems[smallmenu.numitems-1].text = (char*)((lang==1)?"Revenir au shell":"Back to shell");
  while(1) {
    int sres = doMenu(&smallmenu);
    if(sres == MENU_RETURN_SELECTION || sres==KEY_CTRL_EXE) {
      if (smallmenu.selection==smallmenu.numitems){
	return KEY_CTRL_MENU;
      }
      // Attention les entrees sont decalees de 1
      if (smallmenu.selection==1) // tableur
	geoapp(contextptr);
      if (smallmenu.selection==2) // tableur
	sheet(contextptr);
      if (smallmenu.selection==3){ // table periodique
	const char * name,*symbol;
	char protons[32],nucleons[32],mass[32],electroneg[32];
	int res=periodic_table(name,symbol,protons,nucleons,mass,electroneg);
	if (!res)
	  continue;
	char console_buf[64]={0};
	char * ptr=console_buf;
	if (res & 1)
	  ptr=strcpy(ptr,name)+strlen(ptr);
	if (res & 2){
	  if (res & 1)
	    ptr=strcpy(ptr,",")+strlen(ptr);
	  ptr=strcpy(ptr,symbol)+strlen(ptr);
	}
	if (res & 4){
	  if (res&3)
	    ptr=strcpy(ptr,",")+strlen(ptr);
	  ptr=strcpy(ptr,protons)+strlen(ptr);
	}
	if (res & 8){
	  if (res&7)
	    ptr=strcpy(ptr,",")+strlen(ptr);
	  ptr=strcpy(ptr,nucleons)+strlen(ptr);
	}
	if (res & 16){
	  if (res&15)
	    ptr=strcpy(ptr,",")+strlen(ptr);
	  ptr=strcpy(ptr,mass+2)+strlen(ptr);
	  ptr=strcpy(ptr,"_(g/mol)")+8;
	}
	if (res & 32){
	  if (res&31)
	    ptr=strcpy(ptr,",")+strlen(ptr);
	  ptr=strcpy(ptr,electroneg+4)+strlen(ptr);
	}
	copy_clipboard(console_buf,true,true);
	continue; // return Console_Input((const unsigned char*)console_buf);
      }
      if (smallmenu.selection==4){
	finance(-1,contextptr);
	continue;
      }
      if (smallmenu.selection==5){
	finance(1,contextptr);
	continue;
      }
      if (smallmenu.selection==6){
	// Exemple simple d'application tierce: la suite de Syracuse
	// on entre la valeur de u0
	double d; int i;
	for (;;){
	  inputdouble(lang==1?("Suite de Syracuse. u0?"):"Syracuse sequence, u9?",d,contextptr);
	  i=(d);
	  if (i==d)
	    break;
	  confirm(lang==1?("u0 doit etre entier!"):"u0 must be an integer!",gettext("Recommencez"));
	}
	i=max(i,1);
	vecteur v(1,i); // initialise une liste avec u0
	while (i!=1){
	  if (i%2)
	    i=3*i+1;
	  else
	    i=i/2;
	  v.push_back(i);
	}
	// representation graphique de la liste en appelant la commande Xcas listplot
	displaygraph(_listplot(v,contextptr),symbolic(at_plotlist,v),contextptr);
	// on copie la liste dans le presse-papier
	copy_clipboard(gen(v).print(contextptr),true,true);
	continue;
	// return Console_Input((const unsigned char*)gen(v).print(contextptr).c_str());
      }
      if (smallmenu.selection==7) // mastermind, on ne quitte pas
	mastermind(contextptr);
      if (smallmenu.selection==8)
	fractale(contextptr);
    } // end sres==menu_selection
    Console_Disp();
    break;
  } // end endless while
  return CONSOLE_SUCCEEDED;
}

int geoapp(GIAC_CONTEXT){
  int res=newgeo(contextptr);
  if (res<0) return res;
  // load a figure?
  textArea * text=geoptr->hp;
  vector<string> fign,figs;
  vecteur V(gen2vecteur(giac::_VARS(0,contextptr)));
  for (int i=0;i<V.size();++i){
    gen tmp(V[i]);
    gen val=eval(tmp,1,contextptr);
    if (val.type==_VECT && val._VECTptr->size()==2 && val._VECTptr->front()==at_pnt){
      vecteur & v=*val._VECTptr;
      if (v[1].type==_STRNG){
	fign.push_back(tmp.print(contextptr));
	figs.push_back(*v[1]._STRNGptr);
      }
    }
  }
  if (1 || !figs.empty()){
    if (0 && figs.size()==1){
      text->elements.clear();
      add(text,figs[0]);
      text->filename=fign[0];
    }
    else {
      const char * tab[figs.size()+3]={0};
      for (int i=0;i<figs.size();++i)
	tab[i]=fign[i].c_str();
      tab[figs.size()]=lang==1?"Nouvelle figure 2d":"New 2d figure";
      tab[figs.size()+1]=lang==1?"Nouvelle figure 3d":"New 3d figure";
      tab[figs.size()+2]=0;
      int s=select_item(tab,lang==1?"Choisir figure":"Choose figure",true);
      if (s>=0 && s<sizeof(tab)/sizeof(char *) && tab[s]){
	text->elements.clear();
	if (s<figs.size()){
	  add(text,figs[s]);
	  text->filename="\\\\fls0\\";
	  text->filename+=fign[s];
	  text->filename+=".py";
	  geoparse(text,contextptr);
	  geoptr->eval(0);
	}
	else {
	  geoptr->plot_instructions.clear();
	  geoptr->symbolic_instructions.clear();
	  geoptr->is3d=(s==figs.size()+1);
	  geoptr->update_rotation();
	  geoptr->orthonormalize();
	  geoptr->g=0;
	  text->filename="\\\\fls0\\figure";
	  text->filename+=print_INT_(figs.size()+1);
	  text->filename+=".py";
	}
      }
      else return -3;
    }
  }
  return geoloop(geoptr);
}

/* *******************
 *      FLASH        *
 ********************* */
#ifdef NUMWORKS

void flash_info(const char * buf,std::vector<fileinfo_t> &v,size_t & first_modif,bool modif,int initpos,GIAC_CONTEXT){
  Menu smallmenu;
  smallmenu.numitems=v.size();
  MenuItem smallmenuitems[smallmenu.numitems];
  smallmenu.items=smallmenuitems;
  smallmenu.height=modif?11:12;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char*)(lang==1?"Info Flash":"Flash Files");
  smallmenu.type = MENUTYPE_FKEYS;
  smallmenu.selection=initpos;
  if (modif){
    smallmenu.title = (char*)(lang==1?"Modifier fichiers":"Modify files");
  }
  vector<string> vs(v.size());
  for (int i=0;i<v.size();++i){
    vs[i]=v[i].filename.c_str();
    vs[i]+=' ';
    vs[i]+=print_INT_(v[i].size);
    smallmenuitems[i].text=(char *)vs[i].c_str();
    smallmenuitems[i].type = MENUITEM_CHECKBOX;
    smallmenuitems[i].value= ((v[i].mode/100)&4)==4;
  }
  while (1){
    if (modif){
      drawRectangle(0,200,LCD_WIDTH_PX,22,giac::_WHITE);
      os_draw_string(0,200,giac::_WHITE,33333,"Tool: renam | Ans: -/+ | EXE: ok");
    }
    int sres = doMenu(&smallmenu);
    int i=smallmenu.selection-1;
    if (sres==MENU_RETURN_EXIT){
      break;
    }
    if (modif && sres == KEY_CTRL_CATALOG || sres==KEY_BOOK) { // rename
      string s=v[i].filename,msg1=(lang==1?"Renommer ":"Rename ")+s;
      int j=inputline(msg1.c_str(),"",s,false);
      if (j){
	v[i].filename=s;
	vs[i]=v[i].filename.c_str();
	vs[i]+=' ';
	vs[i]+=print_INT_(v[i].size);
	smallmenuitems[i].text=(char *)vs[i].c_str();
      }
      continue;
    }
    if (sres == MENU_RETURN_SELECTION  || sres==KEY_CTRL_EXE) {
      if (modif){
	flash_synchronize(buf,v,&first_modif);
#if defined NUMWORKS && !defined DEVICE
	// debug
	file_savetar("file.tar",(char *)buf,tar_totalsize(buf,0));
#endif
	break;
      }
      string msg1=vs[i];
      const char * ptr=(buf+v[i].header_offset);
#ifdef HAVE_TIME_H
      char tbuf[512];
      char m[4]={0,0,0,0};
      strncpy(m,ptr+104,3);
      string msg2=string("Mode: ")+m;
      ulonglong ul=fromstring8(ptr+136);
      if (ul==(ulonglong) -1 || ul<1e8) // timestamp from calculator last reset
	msg2 += ", RTC " + print_INT_(ul);
      else {
	time_t t=ul;
	tm ts=*localtime(&t);
	strftime(tbuf, sizeof(tbuf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
	msg2 += string(", ")+tbuf;
      }
#else
      string msg2=string("Mode: ")+(ptr+104)+string(" mtime ")+print_INT_(fromstring8(ptr+136));
#endif
      confirm(msg1.c_str(),msg2.c_str());
      continue;
    }
    if (sres==KEY_CHAR_ANS){
      if (i>=0 && i<v.size()){
	if (modif && i>10){
	  smallmenuitems[i].value=!smallmenuitems[i].value;
	  int m=v[i].mode;
	  if (smallmenuitems[i].value)
	    m = ((m/100) | 4)*100+(m%100);
	  else
	    m = ((m/100) & 3)*100+(m%100);
	  v[i].mode=m;
	  if (smallmenuitems[i].value){
	    // uncheck all files having the same filename
	    const string & filename=v[i].filename;
	    for (int j=0;j<v.size();++j){
	      if (j==i || v[j].filename!=filename)
		continue;
	      m=v[j].mode;
	      m = ((m/100) & 3)*100+(m%100);
	      v[j].mode=m;
	      smallmenuitems[j].value=false;
	    }
	  }
	}
      }
    }
  }
}

void flash_info(const char * buf,size_t & first_modif,bool modif,GIAC_CONTEXT){
  std::vector<fileinfo_t> v=tar_fileinfo(buf,0);
  int initpos=1;
  if (modif)
    initpos=v.size();
  flash_info(buf,v,first_modif,modif,initpos,contextptr);
}

// copy text file from ram scriptstore
int flash_from_ram(const char * buf,size_t & first_modif,GIAC_CONTEXT){
  char filename[MAX_FILENAME_SIZE+1];
  int n=fileBrowser(filename,"py",(lang==1?"Choisir fichier a copier":"Select file to copy"));
  if (n==0) return 0;
  const char * data=read_file(filename);
  n=flash_adddata(buf,filename,data,strlen(data),0);
  return n;
}

void handle_flash(GIAC_CONTEXT){
  const char flash_fr[]="Cette application, disponible hors mode examen, permet de sauvegarder et gerer des scripts en memoire flash. Elle a besoin de 70K de memoire RAM, lancez-la tout de suite apres avoir ouvert KhiCAS.\nPour eviter une usure trop rapide de la flash, il est conseille de l'utiliser le moins souvent possible et de ne pas vider la corbeille avant que cela ne soit necessaire (ainsi les nouveaux fichiers s'ecriront sur d'autres secteurs).\nL'auteur decline toute responsabilite en cas d'usure prematuree de votre memoire flash.";
  const char flash_en[]="This app (not available if exam mode is on) lets you save and handle scripts in flash memory. It requires 70K of free RAM, you should run it immediatly after launching KhiCAS.\nIn order to avoid premature wear of your flash, run this app only when required. Don't empty the trash unless it's necessary (that way new files will be written in other sectors).\nThe author declines all responsability in the event of premature wear of your flash memory.";
  textArea text;
  text.editable=false;
  text.clipline=-1;
  text.title =(lang==1)?"EXIT: annuler, EXE: ok":"EXIT: cancel, EXE: run";
  add(&text,(lang==1)?flash_fr:flash_en);
  int key=doTextArea(&text,contextptr);
  if (key!=1
#ifdef DEVICE
      || inexammode()
#endif
      )
    return;
  text.elements.clear();
  buf64k=(char *)malloc(1<<16);
  if (buf64k==0){
    confirm(lang==1?"Pas assez de memoire RAM.":"RAM Memory full",lang==1?"Purgez et relancez KhiCAS":"Purge and restart KhiCAS");    
    return;
  }
#ifndef NUMWORKS
  char * freeptr=0;
  const char * flash_buf=file_gettar_aligned("apps.tar",freeptr);
#endif
  Menu smallmenu;
  smallmenu.numitems=5;
  MenuItem smallmenuitems[smallmenu.numitems];
  smallmenu.items=smallmenuitems;
  smallmenu.height=12;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenuitems[0].text = (char*)(lang==1?"Informations flash":"Flash informations");
  smallmenuitems[1].text = (char*)(lang==1?"Copier RAM->flash":"Copy RAM->flash");
  smallmenuitems[2].text = (char*)(lang==1?"Modifier infos fichiers":"Modify file infos");
  smallmenuitems[3].text = (char*)(lang==1?"Vider la corbeille":"Empty trash");
  smallmenuitems[4].text = (char*)(lang==1?"Quitter":"Leave");
  while (1){
    size_t first_modif=tar_totalsize(flash_buf,numworks_maxtarsize);
    string title=(lang==1?"Flash libre ":"Free flash ");
    title += print_INT_(numworks_maxtarsize-first_modif);
    smallmenu.title = (char*)title.c_str();
    smallmenu.selection = 1;
    int sres = doMenu(&smallmenu);
    if (sres==MENU_RETURN_EXIT){
      break;
    } 
    if (sres == MENU_RETURN_SELECTION  || sres==KEY_CTRL_EXE) {
      if (smallmenu.selection == smallmenu.numitems)
	break;
      if (smallmenu.selection == 1){
	flash_info(flash_buf,first_modif,false,contextptr); // info only, no erase
	continue;
      }
      if (smallmenu.selection == 2){
	if (flash_from_ram(flash_buf,first_modif,contextptr)){
	  // uncheck files having the same filename
	  std::vector<fileinfo_t> v=tar_fileinfo(flash_buf,0);
	  int n=v.size();
	  if (n){
	    --n;
	    string & filename=v[n].filename;
	    int modif=0;
	    for (int j=0;j<n;++j){
	      if ( v[j].filename!=filename)
		continue;
	      modif++;
	      int m=v[j].mode;
	      m = ((m/100) & 3)*100+(m%100);
	      v[j].mode=m;
	    }
	    if (modif)
	      flash_info(flash_buf,v,first_modif,true,v.size(),contextptr);
	  }
	}
	continue;
      }
      if (smallmenu.selection == 3){
	flash_info(flash_buf,first_modif,true,contextptr); // erase files
	continue;
      }
      if (smallmenu.selection==4){
	if (numworks_maxtarsize-first_modif>65536 && do_confirm(lang==1?"Il reste de la place, etes-vous sur?":"There's still room, are you sure?"))
	  flash_emptytrash(flash_buf,&first_modif);
      }
    }
  }
  free(buf64k);
#ifndef DEVICE
  //free(freeptr);
#endif
}
#else
void handle_flash(GIAC_CONTEXT){
  
}
#endif

/* **************************
   * SPREADSHEET CODE       *
   ************************** */
const int row_height=20;
const int col_width=60;
string printcell(int i,int j){
  string s="";
  s+=char('A'+j);
  s+=print_INT_(i);
  return s;
}
string printsel(int r,int c,int R,int C){
  return printcell(r,c)+":"+printcell(R,C);
}

void change_undo(tableur & t){
  t.undo=t.m;
  t.changed=true;
}

void save_sheet(tableur & t,GIAC_CONTEXT){
  t.changed=false;
  statuslinemsg(lang==1?"Sauvegarde en cours":"Saving in progress");
  sync_screen();  
#if 1
  string s=print_tableur(t,contextptr);
#else
  string s=gen(extractmatricefromsheet(t.m,false),_SPREAD__VECT).print(contextptr);
#endif
  string filename("\\\\fls0\\");
  filename += remove_path0(remove_extension(t.filename));
  filename += ".tab";
#ifdef NSPIRE_NEWLIB
  filename+=".tns";
#endif
  save_script(filename.c_str(),s);
}
void sheet_status(tableur & t,GIAC_CONTEXT){
  string st;
  if (python_compat(contextptr))
    st="tabl Py ";
  else
    st="tabl Xcas ";
  if (t.var.type==_IDNT)
    st += t.var.print(contextptr);
  else
    st += "<>";
  st += ' ';
  st += t.filename ;
  st += " R";
  st += print_INT_(t.nrows);
  st += " C";
  st += print_INT_(t.ncols);
  if (t.changed)
    st += " *";
  else
    st += " -";
  if (t.sel_row_begin>=0)
    st += (lang==1)?" esc: annule selection":" esc: cancel selection";
  else {
    if (t.cmd_row>=0)
      st += (lang==1)?" esc: annule ligne cmd":" esc: cancel cmdline";
  }
  statuslinemsg(st.c_str());
}
bool sheet_display(tableur &t,GIAC_CONTEXT){
  int disp_rows=(LCD_HEIGHT_PX-24)/row_height-3;
  int disp_cols=LCD_WIDTH_PX/(col_width+4)-1;
  if (t.disp_row_begin>t.cur_row)
    t.disp_row_begin=t.cur_row;
  if (t.disp_row_begin<t.cur_row-disp_rows+1)
    t.disp_row_begin=t.cur_row-disp_rows+1;
  if (t.disp_col_begin>t.cur_col)
    t.disp_col_begin=t.cur_col;
  if (t.disp_col_begin<t.cur_col-disp_cols+1)
    t.disp_col_begin=t.cur_col-disp_cols+1;
  int I=giacmin(giacmin(t.nrows,t.m.size()),t.disp_row_begin+disp_rows);
  bool has_sel=t.sel_row_begin>=0 && t.sel_row_begin<t.nrows;
  int sel_r=t.sel_row_begin,sel_R=t.cur_row,sel_c=t.sel_col_begin,sel_C=t.cur_col;
  if (sel_r>sel_R)
    swapint(sel_r,sel_R);
  if (sel_c>sel_C)
    swapint(sel_c,sel_C);
  bool has_cmd=t.cmd_row>=0 && t.cmd_row<t.nrows;
  //waitforvblank();
  drawRectangle(0,24,LCD_WIDTH_PX,row_height,_WHITE); // clear column indices row
  string s;
  if (has_sel)
    s=printsel(sel_r,sel_c,sel_R,sel_C);
  else
    s=printcell(t.cur_row,t.cur_col);
  os_draw_string(2,24,_BLACK,_WHITE,s.c_str(),false);
  int y=24+row_height;
  int x=col_width;
  int J=giacmin(t.ncols,t.disp_col_begin+disp_cols);
  for (int j=t.disp_col_begin;j<J;++j){
    draw_line(x,24,x,24+row_height,_BLACK);
    char colname[3]="A"; 
    if (j>=26){ // if we accept more than 26 cols
      colname[0] += j/26;
      colname[1] = 'A'+(j%26);
      colname[2]=0;
    }
    else
      colname[0] += (j % 26);
    os_draw_string(x+col_width/2-4,24,_BLACK,_WHITE,colname);
    x+=col_width+4;
  }
  int waitn=2;
  for (int i=t.disp_row_begin;i<I;++i){
    //if ( (i-t.disp_row_begin) % waitn==waitn-1) waitforvblank();
    drawRectangle(0,y,LCD_WIDTH_PX,row_height,_WHITE); // clear current row
    // draw_line(0,y,LCD_WIDTH_PX,y,_BLACK);
    os_draw_string(4,y,_BLACK,_WHITE,print_INT_(i).c_str()); // row number
    gen g=t.m[i];
    if (g.type!=_VECT)
      return false;
    vecteur & v=*g._VECTptr;
    int J=giacmin(t.ncols,v.size());
    J=giacmin(J,t.disp_col_begin+disp_cols);
    x=col_width;
    for (int j=t.disp_col_begin;j<J;++j){
      draw_line(x,y,x,y+row_height,_BLACK);
      gen vj=v[j];
      if (vj.type<_IDNT)
	vj=makevecteur(vj,vj,0);
      if (vj.type==_VECT && vj._VECTptr->size()==3){
	bool iscur=i==t.cur_row && j==t.cur_col;
	string s;
	if (iscur){
	  if (!has_cmd)
	    t.cmdline=(*vj._VECTptr)[0].print(contextptr);
	}
	bool rev=has_sel?(sel_r<=i && i<=sel_R && sel_c<=j && j<=sel_C):iscur;
	if (rev)
	  drawRectangle(x+1,y,col_width+4,row_height,color_gris);	  
	s=(*vj._VECTptr)[1].print(contextptr);
	int dx=os_draw_string(0,0,0,0,s.c_str(),true); // find width
	if (dx<col_width)
	  os_draw_string(x+2,y+1,_BLACK,rev?color_gris:_WHITE,s.c_str(),false); // draw
	else {
	  if (iscur && !has_sel && t.cmd_row<0)
	    statuslinemsg(s.c_str());
	  s=s.substr(0,8)+"...";
	  os_draw_string_small(x+2,y+2,_BLACK,rev?color_gris:_WHITE,s.c_str(),false); // draw
	}
      }
      x+=col_width+4;
    }
    draw_line(0,y,LCD_WIDTH_PX,y,_BLACK);
    y+=row_height;
  }
  //waitforvblank();
  drawRectangle(0,y,LCD_WIDTH_PX,LCD_HEIGHT_PX-y,_WHITE); // clear cmdline
  draw_line(0,y,LCD_WIDTH_PX,y,_BLACK);
  // commandline
  int p=python_compat(contextptr); python_compat(0,contextptr);
  int xpe=xcas_python_eval; xcas_python_eval=0;
  s=t.cmdline;
  int dx=os_draw_string(0,0,0,0,s.c_str(),true),xend=2; // find width
  bool small=false;//t.keytooltip || dx>=LCD_WIDTH_PX-50;
  int sheety=LCD_HEIGHT_PX-2*row_height,xtooltip=0;
  if (t.cmd_row>=0 && t.cmd_pos>=0 && t.cmd_pos<=s.size()){
    xend=os_draw_string(xend,sheety,_BLUE,_WHITE,printcell(t.cmd_row,t.cmd_col).c_str())+5;
    if (t.disp_start>t.cmd_pos-10)
      t.disp_start=t.cmd_pos-20;
    if (t.disp_start<0)
      t.disp_start=0;
    if (t.cmd_pos-t.disp_start>30)
      t.disp_start=t.cmd_pos-20;
    string s1=s.substr(t.disp_start,t.cmd_pos-t.disp_start);
#if 1
    xtooltip=xend=print_color(xend,sheety-24,s1.c_str(),_BLACK,false,small,contextptr);
#else
    if (small)
      xend=os_draw_string_small(xend,sheety,_BLACK,_WHITE,s1.c_str(),false);
    else
      xend=os_draw_string(xend,sheety,_BLACK,_WHITE,s1.c_str(),false);
#endif
    drawRectangle(xend+1,sheety+2,2,small?10:13,_BLACK);
    xend+=4;
    s=s.substr(t.cmd_pos,s.size()-t.cmd_pos);
    if (has_sel){
      s1=printsel(sel_r,sel_c,sel_R,sel_C);
      xend=os_draw_string(xend,sheety,_BLACK,color_gris,s1.c_str(),false);
      // xend=os_draw_string_small(xend,sheety,_BLACK,color_gris,s1.c_str(),false);
    }
    else {
      if (t.cmd_row!=t.cur_row || t.cmd_col!=t.cur_col){
	xend=os_draw_string(xend,sheety,_BLACK,color_gris,printcell(t.cur_row,t.cur_col).c_str(),false);
	// xend=os_draw_string_small(xend,sheety,_BLACK,color_gris,printcell(t.cur_row,t.cur_col).c_str(),false);
      }
    }
  } // end cmdline active
  else
    xend=os_draw_string(xend,sheety,_BLACK,_WHITE,printcell(t.cur_row,t.cur_col).c_str())+5;    
  int bg=t.cmd_row>=0?_WHITE:57051;
#if 1
    xend=print_color(xend,sheety-24,s.c_str(),_BLACK,false,small,contextptr);
#else
  if (small)
    xend=os_draw_string_small(xend,sheety,_BLACK,bg,s.c_str(),false);
  else
    xend=os_draw_string(xend,sheety,_BLACK,bg,s.c_str(),false);
#endif
  // if (t.keytooltip) t.keytooltip=tooltip(xtooltip,sheety,t.cmd_pos,t.cmdline.c_str(),contextptr); // FIXME
  python_compat(p,contextptr); xcas_python_eval=xpe;
  // fast menus
  //string menu("shift-1 stat1d|2 2d|3 seq|4 edit|5 view|6 graph|7 R|8 list| ");
  //bg=65039;// bg=52832;
  //drawRectangle(0,181,LCD_WIDTH_PX,17,bg);
  //os_draw_string_small(0,205,_BLACK,bg,menu.c_str());
  //int px=0*3,py=58*3;
  //PrintMini(&px,&py,(unsigned char *)(lang?" outil | stat | edit | cmds | A<>a | menu":" tools | stat | edit | cmds | A<>a | menu"),0/*mode*/,0xFFFFFFFF,0,0,COLOR_BLACK, COLOR_ORANGE, 1, 0);
  return true;
}

void activate_cmdline(tableur & t){
  if (t.cmd_row==-1){
    t.cmd_row=t.cur_row;
    t.cmd_col=t.cur_col;
    t.cmd_pos=t.cmdline.size();
    t.disp_start=-1;
  }
}

bool sheet_eval(tableur & t,GIAC_CONTEXT,bool ckrecompute=true){
  t.changed=true;
  if (!ckrecompute || t.recompute)
    spread_eval(t.m,contextptr);
  return true;
}

void fix_sheet(tableur & t,GIAC_CONTEXT){
  for (int i=0;i<t.nrows;++i){
    vecteur & v = *t.m[i]._VECTptr;
    for (int j=0;j<t.ncols;++j){
      gen & g=v[j];
      if (g.type==_VECT){
	vecteur & w=*g._VECTptr;
	if (w[0].type==_SYMB){
	  // cout << "fix " << w[0] << "\n";
	  w[0]=spread_convert(w[0],i,j,contextptr);
	}
      }
    }
  }
}

void copy_right(tableur & t,GIAC_CONTEXT){
  int R=t.cur_row,C=t.cur_col,c=t.ncols;
  vecteur v=*t.m[R]._VECTptr;
  gen g=v[C];
  for (int i=C+1;i<c;++i){
    v[i]=freecopy(g);
  }
  t.m[R]=v;
  sheet_eval(t,contextptr,true);
}

void copy_down(tableur & t,GIAC_CONTEXT){
  int R=t.cur_row,C=t.cur_col,r=giacmin(t.nrows,t.m.size());
  gen g=t.m[R][C];
  for (int i=R+1;i<r;++i){
    vecteur v=*t.m[i]._VECTptr;
    v[C]=freecopy(g);
    t.m[i]=v;
  }
  sheet_eval(t,contextptr,true);
}

void paste(tableur & t,const matrice & m,GIAC_CONTEXT){
  int r=t.cur_row,c=t.cur_col,R=t.nrows,C=t.ncols;
  int dr=t.clip.size(),dc=0;
  if (r+dr>R)
    dr=R-r;
  if (dr && ckmatrix(m,true)){
    dc=m.front()._VECTptr->size();
    if (c+dc>C)
      dc=C-c;
    if (dc){
      for (int i=0;i<dr;++i){
	const vecteur & w=*m[i]._VECTptr;
	vecteur v=*t.m[r+i]._VECTptr;
	for (int j=0;j<dc;++j)
	  v[c+j]=w[j];
	t.m[r+i]=v;
      }
    }
  }
  sheet_eval(t,contextptr,true);
}

void paste(tableur & t,GIAC_CONTEXT){
  paste(t,t.clip,contextptr);
}

void sheet_pntv(const vecteur & v,vecteur & res);
void sheet_pnt(const gen & g,vecteur & res){
  if (g.type==_VECT)
    sheet_pntv(*g._VECTptr,res);
  if (g.is_symb_of_sommet(at_pnt))
    res.push_back(g);
}

void sheet_pntv(const vecteur & v,vecteur & res){
  for (int i=0;i<v.size();++i){
    sheet_pnt(v[i],res);
  }
}

void resizesheet(tableur &t){
  int cur_r=t.m.size(),cur_c=t.m.front()._VECTptr->size(),nr=t.nrows,nc=t.ncols;
  if (nr!=cur_r || nc!=cur_c){
    if (do_confirm(((lang==1?"Redimensionner ":"Resize ")+print_INT_(cur_r)+"x"+print_INT_(cur_c)+"->"+print_INT_(nr)+"x"+print_INT_(nc)).c_str())){
      vecteur fill(3,0);
      if (nr<cur_r) // erase rows
	t.m.resize(nr);
      else {
	for (;cur_r<nr;++cur_r){
	  vecteur tmp;
	  for (int j=0;j<nc;++j)
	    tmp.push_back(freecopy(fill));
	  t.m.push_back(tmp);
	}
      }
      for (int i=0;i<nr;++i){
	vecteur & v=*t.m[i]._VECTptr;
	int cur_c=v.size();
	if (nc<cur_c){
	  t.m[i]=vecteur(v.begin(),v.begin()+nc);
	}
	else {
	  for (;cur_c<nc;++cur_c)
	    v.push_back(freecopy(fill));
	}
      }
      t.cur_row=giacmin(t.cur_row,t.nrows);
      t.cur_col=giacmin(t.cur_col,t.ncols);
      t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
    } // end confirmed table resize
    else {
      t.nrows=cur_r;
      t.ncols=cur_c;
    }
  }  
}

void sheet_menu_setup(tableur & t,GIAC_CONTEXT){
  Menu smallmenu;
  smallmenu.numitems=7;
  MenuItem smallmenuitems[smallmenu.numitems];
  smallmenu.items=smallmenuitems;
  smallmenu.height=8;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char*)(lang==1?"Configuration tableur":"Sheet config");
  smallmenuitems[3].type = MENUITEM_CHECKBOX;
  smallmenuitems[3].text = (char*)"Reeval";
  smallmenuitems[4].type = MENUITEM_CHECKBOX;
  smallmenuitems[4].text = (char*)(lang==1?"Matrice remplit":"Matrix fill cells");
  smallmenuitems[5].type = MENUITEM_CHECKBOX;
  smallmenuitems[5].text = (char*)(lang==1?"Deplace vers bas":"Move down");
  smallmenuitems[smallmenu.numitems-1].text = (char*) "Quit";
  while(1) {
    string dig("Digits (in Xcas): ");
    dig += print_INT_(decimal_digits(contextptr));
    smallmenuitems[0].text = (char*)dig.c_str();
    string nrows((lang==1?"Lignes ":"Rows ")+print_INT_(t.nrows));
    smallmenuitems[1].text = (char*)nrows.c_str();
    string ncols((lang==1?"Colonnes ":"Cols ")+print_INT_(t.ncols));
    smallmenuitems[2].text = (char*)ncols.c_str();
    smallmenuitems[3].value = t.recompute;
    smallmenuitems[4].value = t.matrix_fill_cells;
    smallmenuitems[5].value = t.movedown;
    int sres = doMenu(&smallmenu);
    if (sres==MENU_RETURN_EXIT){
      resizesheet(t);
      break;
    }
    if (sres == MENU_RETURN_SELECTION  || sres==KEY_CTRL_EXE) {
      if (smallmenu.selection == 1){
	double d=decimal_digits(contextptr);
	if (inputdouble("Nombre de digits?",d,contextptr) && d==int(d) && d>0){
	  decimal_digits(d,contextptr);
	}
	continue;
      }
      if (smallmenu.selection == 2){
	double d=t.nrows;
	if (inputdouble((lang==1?"Nombre de lignes?":"Rows?"),d,contextptr) && d==int(d) && d>0){
	  t.nrows=d;
	}
	continue;
      }
      if (smallmenu.selection == 3){
	double d=t.ncols;
	if (inputdouble((lang==1?"Nombre de colonnes?":"Colonnes?"),d,contextptr) && d==int(d) && d>0){
	  t.ncols=d;
	}
	continue;
      }
      if (smallmenu.selection == 4){
	t.recompute=!t.recompute;
	continue;
      }
      if (smallmenu.selection==5){
	t.matrix_fill_cells=!t.matrix_fill_cells;
	continue;
      }
      if (smallmenu.selection == 6){
	t.movedown=!t.movedown;
	continue;
      }
      if (smallmenu.selection == smallmenu.numitems){
	change_undo(t);
	resizesheet(t);
	break;
      }	
    }      
  } // end endless while
}

void sheet_graph(tableur &t,GIAC_CONTEXT){
  vecteur v;
  sheet_pnt(t.m,v);
  gen g(v);
  check_do_graph(g,0,2,contextptr);
}

int sheet_menu_menu(tableur & t,GIAC_CONTEXT){
  t.cmd_row=-1; t.cmd_pos=-1; t.sel_row_begin=-1;
  Menu smallmenu;
  smallmenu.numitems=14;
  MenuItem smallmenuitems[smallmenu.numitems];
  smallmenu.items=smallmenuitems;
  smallmenu.height=8;
  //smallmenu.width=24;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
#ifdef NUMWORKS
  smallmenu.title = (char*)(lang==1?"Back: annule menu tableur":"Back: cancel sheet menu");
#else
  smallmenu.title = (char*)(lang==1?"Exit: annule menu tableur":"Exit: cancel sheet menu");
#endif
  smallmenuitems[0].text = (char *)(lang==1?"Sauvegarde tableur (shift sto)":"Save sheet (shift sto)");
  smallmenuitems[1].text = (char *)(lang==1?"Sauvegarder comme":"Save sheet as");
  //if (nspire_exam_mode==2) smallmenuitems[1].text=smallmenuitems[0].text = (char*)(lang==1?"Sauvegarde desactivee":"Saving disabled");
  smallmenuitems[2].text = (char*)(lang==1?"Charger":"Load");
  string cell=(lang==1?"Editer cellule ":"Edit cell ")+printcell(t.cur_row,t.cur_col);
  smallmenuitems[3].text = (char*)cell.c_str();
  smallmenuitems[4].text = (char*)(lang==1?"Graphique (S<->D)":"Graph (S<->D)");
#ifdef NUMWORKS
  smallmenuitems[5].text = (char*)(lang==1?"Copie vers le bas":"Copy down");
  smallmenuitems[6].text = (char*)(lang==1?"Copie vers droite":"Copy right");
#else
  smallmenuitems[5].text = (char*)(lang==1?"Copie vers le bas":"Copy down (ctrl D)");
  smallmenuitems[6].text = (char*)(lang==1?"Copie vers la droite":"Copy right (ctrl R)");
#endif
  smallmenuitems[7].text = (char*)(lang==1?"Inserer 1 ligne":"Insert row");
  smallmenuitems[8].text = (char*)(lang==1?"Inserer 1 colonne":"Insert column");
  smallmenuitems[9].text = (char*)(lang==1?"Efface ligne courante":"Remove current row");
  smallmenuitems[10].text = (char*)(lang==1?"Efface col. courante":"Remove current column");
  smallmenuitems[11].text = (char*)(lang==1?"Remplir de 0":"Fill sheet with 0");
  smallmenuitems[smallmenu.numitems-2].text = (char*) "Config";
  smallmenuitems[smallmenu.numitems-1].text = (char*) (lang==1?"Quitter tableur":"Leave sheet");
  while(1) {
    int sres = doMenu(&smallmenu);
    if (sres==MENU_RETURN_EXIT)
      return -1;
    if (sres == MENU_RETURN_SELECTION  || sres==KEY_CTRL_EXE) {
      if (smallmenu.selection == 1){
	// save
	save_sheet(t,contextptr);
	return -1;
      }
      if (smallmenu.selection == 2 ){
	// save
	char buf[270];
	if (get_filename(buf,".tab")){
	  t.filename=remove_path0(remove_extension(buf));
	  save_sheet(t,contextptr);
	  return -1;
	}
      }
      if (smallmenu.selection== 3
	  && !inexammode
	  ) {
	char filename[128];
	if (fileBrowser(filename,(char *)"*.tab",(lang==1?"Fichiers tableurs":"Sheet files"))){
	  if (t.changed && do_confirm(lang==1?"Sauvegarder le tableur actuel?":"Save current sheet?"))
	    save_sheet(t,contextptr);
	  const char * s=read_file(filename);
	  if (s){
	    gen g(s,contextptr);
	    g=eval(g,1,contextptr);
	    if (ckmatrix(g,true)){
	      t.filename=remove_path0(filename);
	      t.m=*g._VECTptr;
	      t.nrows=t.m.size();
	      t.ncols=t.m.front()._VECTptr->size();
	      fix_sheet(t,contextptr);
	      t.cur_col=t.cur_row=0;
	      t.sel_row_begin=t.cmd_row=-1;
	    }
	    else
	      s=0;
	  }
	  if (!s)
	    do_confirm(lang==1?"Erreur de lecture du fichier":"Error reading file");
	}
	return -1;
      } // end load
      if (smallmenu.selection==4){
	activate_cmdline(t);
	t.cmd_pos=t.cmdline.size();
	return -1;
      }
      if (smallmenu.selection==5){
	sheet_graph(t,contextptr);
	return -1;
      }
      if (smallmenu.selection==6){
	t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	copy_down(t,contextptr);
	return -1;
      }
      if (smallmenu.selection==7){
	t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	copy_right(t,contextptr);
	return -1;
      }
      if (smallmenu.selection==8){
	t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	change_undo(t);
	t.m=matrice_insert(t.m,t.cur_row,t.cur_col,1,0,makevecteur(0,0,2),contextptr);
	t.nrows++;
	return -1;
      }
      if (smallmenu.selection==9){
	t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	change_undo(t);
	t.m=matrice_insert(t.m,t.cur_row,t.cur_col,0,1,makevecteur(0,0,2),contextptr);
	t.ncols++;
	return -1;
      }
      if (smallmenu.selection==10 && t.nrows>=2){
	t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	change_undo(t);
	t.m=matrice_erase(t.m,t.cur_row,t.cur_col,1,0,contextptr);
	--t.nrows;
	return -1;
      }
      if (smallmenu.selection==11 && t.ncols>=2){
	t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	change_undo(t);
	t.m=matrice_erase(t.m,t.cur_row,t.cur_col,0,1,contextptr);
	--t.ncols;
	return -1;
      }
      if (smallmenu.selection==12){
	t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	change_undo(t);
	gen g=vecteur(t.ncols);
	t.m=makefreematrice(vecteur(t.nrows,g));
	makespreadsheetmatrice(t.m,contextptr);
	return -1;
      }
      if (smallmenu.selection == smallmenu.numitems-1){
	sheet_menu_setup(t,contextptr);
	continue;
      }
      if (smallmenu.selection == smallmenu.numitems){
	return 0;
      }
    }
  } // end endless while
  return 1;
}

bool is_empty_cell(const gen & g){
  if (g.type==_VECT) return is_zero(g[0]);
  return is_zero(g);
}

void sheet_cmd(tableur & t,const char * ans){
  string s=ans; 
  if (t.sel_row_begin>=0){
    t.cmdline="";
    s="="+s+"matrix("+print_INT_(absint(t.sel_row_begin-t.cur_row)+1)+","+print_INT_(absint(t.sel_col_begin-t.cur_col)+1)+","+printsel(t.sel_row_begin,t.sel_col_begin,t.cur_row,t.cur_col)+")";
    if (t.cur_row<t.sel_row_begin)
      t.cur_row=t.sel_row_begin;
    t.sel_row_begin=-1;
    if (t.cur_col<t.sel_col_begin)
      t.cur_col=t.sel_col_begin;
    int i,j=t.cur_col;
    // find empty cell in next rows
    for (i=t.cur_row+1;i<t.nrows;++i){
      if (is_empty_cell(t.m[i][t.cur_col]))
	break;
    }
    if (i==t.nrows){
      // find an empty cell in next columns
      for (j=t.cur_col+1;j<t.ncols;++j){
	for (i=0;i<t.nrows;++i){
	  if (is_empty_cell(t.m[i][j]))
	    break;
	}
	if (i<t.nrows)
	  break;
      }
    }
    if (i<t.nrows && j<t.ncols){
      t.cur_row=i;
      t.cur_col=j;
    }
    else {
      do_confirm((lang==1?"Impossible de trouver une cellule libre":"Could not find an empty cell"));
      return;
    }
  }
  activate_cmdline(t);
  insert(t.cmdline,t.cmd_pos,s.c_str());
  t.cmd_pos += s.size();
  t.keytooltip=true;
}

void sheet_cmdline(tableur &t,GIAC_CONTEXT){
  gen g(t.cmdline,contextptr);
  change_undo(t);
  bool doit=true;
  bool tableseq=g.is_symb_of_sommet(at_tableseq);
  bool tablefunc=g.is_symb_of_sommet(at_tablefunc);
  if (tableseq || t.matrix_fill_cells){
    set_abort();
    gen g1=protecteval(g,1,contextptr);
    clear_abort();
    if (g1.type==_VECT){
      doit=false;
      matrice & m=*g1._VECTptr;
      if (!ckmatrix(m)){
	m=vecteur(1,m);
	if (t.movedown)
	  m=mtran(m);
      }
      matrice clip=t.clip;
      makespreadsheetmatrice(m,contextptr);
      t.clip=m;
      paste(t,contextptr);
      t.clip=clip;
      if (tableseq && t.cur_row+4<t.nrows){
	t.cur_row += 4;
	copy_down(t,contextptr);
      }
      if (tablefunc && t.cur_row+3<t.nrows && t.cur_col+1<t.ncols){
	t.cur_row += 3;
	copy_down(t,contextptr);
	t.cur_col++;
	copy_down(t,contextptr);
      }
    }
  }
  if (doit) {
    if (t.cmd_row<t.m.size()){
      gen v=t.m[t.cmd_row];
      if (v.type==_VECT && t.cmd_col>=0 && t.cmd_col<v._VECTptr->size()){
	vecteur w=*v._VECTptr;
	g=spread_convert(g,t.cur_row,t.cur_col,contextptr);
	w[t.cmd_col]=makevecteur(g,g,0);
	t.m[t.cmd_row]=w;
	sheet_eval(t,contextptr,true);
      }
    }
  }
  t.cur_row=t.cmd_row;
  t.cur_col=t.cmd_col;
  t.cmd_row=-1;
  t.cmd_pos=-1;
  if (t.movedown){
    ++t.cur_row;
    if (t.cur_row>=t.nrows){
      t.cur_row=0;
      ++t.cur_col;
      if (t.cur_col>=t.ncols)
	t.cur_col=0;
    }
  }
  else {
    ++t.cur_col;
    if (t.cur_col>=t.ncols){
      t.cur_col=0;
      ++t.cur_row;
      if (t.cur_row>=t.nrows){
	t.cur_row=0;
      }
    }
  }
}

void sheet_help_insert(tableur & t,int exec,GIAC_CONTEXT){
  int back;
  string adds=help_insert(t.cmdline.substr(0,t.cmd_pos).c_str(),back,exec);
  if (back>=t.cmd_pos){
    t.cmdline=t.cmdline.substr(0,t.cmd_pos-back)+t.cmdline.substr(t.cmd_pos,t.cmdline.size()-t.cmd_pos);
    t.cmd_pos-=back;
  }
  if (!adds.empty())
    sheet_cmd(t,adds.c_str());
}

giac::gen sheet(GIAC_CONTEXT){
  if (!xcas_sheetptr)
    xcas_sheetptr=new_tableur(contextptr);
  tableur & t=*xcas_sheetptr;
  sheet_eval(t,contextptr,true);
  t.changed=false;
  bool status_freeze=false;
  t.keytooltip=false;
  const char tmenu[]= "F1 seq\nrange(\nrandvector(\nrandmatrix(\n\nseq(\ntableseq(\ntablefunc(\nscatterplot(\nlinear_regression_plot(\nF2 stat\nsum(\nmean(\nstddev(\nmedian(\nhistogram(\nbarplot(\nboxwhisker(\ncamembert(\nF3 edit\n$\nchar table\nedit_cell\nundo\ncopy_down\ncopy_right\ninsert_row\ninsert_col\nF4 cmds\nF5 none\nF6 graph\nreserved\nF7 arit\n mod \nirem(\nifactor(\ngcd(\nisprime(\nnextprime(\npowmod(\niegcd(\nF8 real\nexact(\napprox(\nfloor(\nceil(\nround(\nsign(\nmax(\nmin(\nF9 cplx\nabs(\narg(\nre(\nim(\nconj(\ncsolve(\ncfactor(\ncpartfrac(\nF: plot\nplot(\nplotseq(\nplotlist(\nplotparam(\nplotpolar(\nplotfield(\nhistogram(\nbarplot(\nF@ poly\nproot(\npcoeff(\nquo(\nrem(\ngcd(\negcd(\nabcuv\nresultant(\nF< prg1\n;\n:\n&\n|\n<\n>\nf(x):=\nchar table\nFB prg2\nifte(\nif\nthen\nelse\nend\n<\n>\nfor from to do\nF= 2d  \nlinear_regression_plot(\nlogarithmic_regression_plot(\nexponential_regression_plot(\npower_regression_plot(\npolynomial_regression_plot(\nsin_regression_plot(\nscatterplot(\npolygonscatterplot(\nF? color\nred\nblue\ngreen\ncyan\nyellow\nmagenta\nblack\nwhite\nF; misc\n<\n>\n_\n!\n % \nrand(\nbinomial(\nnormald(\nFA geo\npoint(\nline(\ntriangle(\ncircle(\nplane(\nsphere(\ncube(\nicosahedron(\nF> lin\nmatrix(\ndet(\nmatpow(\nranm(\ntran(\nrref(\negvl(\negv(\nFC list\nmakelist(\nrange(\nseq(\nsize(\nappend(\nranv(\nsort(\napply(\nFD poly\nproot(\npcoeff(\nquo(\nrem(\ngcd(\negcd(\nresultant(\nGF(\n";
  update_fmenu((const unsigned char *) tmenu);
  for (;;){
    int R=t.cur_row,C=t.cur_col;
    if (t.cmd_row>=0){
      R=t.cmd_row;
      C=t.cmd_col;
    }
    printcell_current_row(contextptr)=R;
    printcell_current_col(contextptr)=C;
    if (!status_freeze)
      sheet_status(t,contextptr);
    sheet_display(t,contextptr);
    casiostatus(0,0,0,0); // always rewrite menu
    int key;
    string menu,shiftmenu,alphamenu; int menucolorbg=12345;
    get_current_console_menu(menu,shiftmenu,alphamenu,menucolorbg,3);
    in_ckgetkey(&key,1,menu.c_str(),shiftmenu.c_str(),alphamenu.c_str(),menucolorbg);
    if (key==KEY_SAVE)
      key=':';
    if (key==KEY_CTRL_F5){
      handle_f5();
      continue;
    }
    if (key==KEY_SHUTDOWN)
      return key;
    if (0
	//t.keytooltip // FIXME
	){
      t.keytooltip=false; 
      if (key==KEY_CTRL_EXIT)
	continue;
      if (key==KEY_CTRL_RIGHT && t.cmd_pos==t.cmdline.size())
	key=KEY_CTRL_EXE;
      if (key==KEY_CTRL_DOWN || key==KEY_CTRL_VARS)
	key=KEY_BOOK;
      if (key==KEY_CTRL_EXE || key==KEY_CHAR_ANS){
	sheet_help_insert(t,key,contextptr);
	continue;
      }
    }
    status_freeze=false;
    if (key==KEY_CTRL_SETUP){
      sheet_menu_setup(t,contextptr);
      continue;
    }
    if (key==KEY_CHAR_STORE && t.cmd_row<0){
      save_sheet(t,contextptr);
      continue;
    }
    if (key==KEY_CTRL_F6){
      if (sheet_menu_menu(t,contextptr)==0)
	return 0;
    }
    if (key==KEY_CTRL_CAPTURE){
      save_sheet(t,contextptr);
      continue;
    }
    if (key==KEY_CTRL_EXIT){
      if (t.sel_row_begin>=0){
	t.sel_row_begin=-1;
	continue;
      }
      if (t.cmd_row>=0){
	bool b= t.cmd_row==t.cur_row && t.cmd_col==t.cur_col;
	t.cur_row=t.cmd_row;
	t.cur_col=t.cmd_col;
	if (b)
	  t.cmd_row=-1;
	continue;
      }
      if (!t.changed || do_confirm("Quit?"))
	return 0;
    }
    if (key==KEY_CTRL_INS){
      int c=chartab();
      if (c>=0)
	key=c;
    }
    switch (key){
    case KEY_CTRL_UNDO:
      std::swap(t.m,t.undo);
      sheet_eval(t,contextptr);
      continue;
    case KEY_CTRL_CLIP:
      if (t.sel_row_begin<0){
	t.sel_row_begin=t.cur_row;
	t.sel_col_begin=t.cur_col;
      }
      else {
	int r=t.cur_row,R=t.sel_row_begin,c=t.cur_col,C=t.sel_col_begin;
	if (r>R)
	  swapint(r,R);
	if (c>C)
	  swapint(c,C);
	t.clip=matrice_extract(t.m,r,c,R-r+1,C-c+1);
	copy_clipboard(gen(extractmatricefromsheet(t.clip)).print(contextptr).c_str(),true,true);
	t.sel_row_begin=-1;
      }
      continue;
    case KEY_CTRL_PASTE:
      paste(t,contextptr);
      status_freeze=true;
      continue;
    case KEY_SELECT_RIGHT:
      if (t.sel_row_begin<0){
	t.sel_row_begin=t.cur_row;
	t.sel_col_begin=t.cur_col;
      }
    case KEY_CTRL_RIGHT:
      if (t.cmd_pos>=0 && t.cmd_row==t.cur_row && t.cmd_col==t.cur_col && t.sel_row_begin==-1){
	++t.cmd_pos;
	if (t.cmd_pos>t.cmdline.size())
	  t.cmd_pos=t.cmdline.size();
      }
      else {
	++t.cur_col;
	if (t.cur_col>=t.ncols)
	  t.cur_col=0;
      }
      continue;
    case KEY_SHIFT_RIGHT:
      if (t.cmd_pos>=0 && t.cmd_row==t.cur_row && t.cmd_col==t.cur_col && t.sel_row_begin==-1){
	t.cmd_pos=t.cmdline.size();
      }
      else 
	t.cur_col=t.ncols-1;
      break;
    case KEY_SELECT_LEFT:
      if (t.sel_row_begin<0){
	t.sel_row_begin=t.cur_row;
	t.sel_col_begin=t.cur_col;
      }
    case KEY_CTRL_LEFT:
      if (t.cmd_pos>=0 && t.cmd_row==t.cur_row && t.cmd_col==t.cur_col && t.sel_row_begin==-1){
	if (t.cmd_pos>0)
	  --t.cmd_pos;
      }
      else {
	--t.cur_col;
	if (t.cur_col<0)
	  t.cur_col=t.ncols-1;
      }
      continue;
    case KEY_SHIFT_LEFT:
      if (t.cmd_pos>=0 && t.cmd_row==t.cur_row && t.cmd_col==t.cur_col && t.sel_row_begin==-1){
	t.cmd_pos=0;
      }
      else {
	t.cur_col=0;
      }
      break;
    case KEY_SELECT_UP:
      if (t.sel_row_begin<0){
	t.sel_row_begin=t.cur_row;
	t.sel_col_begin=t.cur_col;
      }
    case KEY_CTRL_UP:
      --t.cur_row;
      if (t.cur_row<0)
	t.cur_row=t.nrows-1;
      continue;
    case KEY_SELECT_DOWN:
      if (t.sel_row_begin<0){
	t.sel_row_begin=t.cur_row;
	t.sel_col_begin=t.cur_col;
      }
    case KEY_CTRL_DOWN:
      ++t.cur_row;
      if (t.cur_row>=t.nrows)
	t.cur_row=0;
      continue;
    case KEY_CTRL_DEL:
      if (t.cmd_row>=0){
	if (t.cmd_pos>0){
	  t.cmdline.erase(t.cmdline.begin()+t.cmd_pos-1);
	  --t.cmd_pos;
	  t.keytooltip=true;
	}
      }
      else {
	t.cmdline="";
	t.cmd_row=t.cur_row;
	t.cmd_col=t.cur_col;
	t.cmd_pos=0;
      }
      continue;
    case KEY_CHAR_CR:
#if 1
      if (t.cmd_row<0){
	sheet_eval(t,contextptr);
	continue;
      }
#else
      if (t.cmd_row<0){
	int r=t.sel_row_begin;
	if (r<0)
	  return extractmatricefromsheet(t.m);
	int R=t.cur_row,c=t.sel_col_begin,C=t.cur_col;
	if (r>R)
	  swapint(r,R);
	if (c>C)
	  swapint(c,C);
	return extractmatricefromsheet(matrice_extract(t.m,r,c,R-r+1,C-c+1));
      }
#endif
    case KEY_CTRL_EXE:
      if (t.cmd_row>=0){
	string s;
	if (t.sel_row_begin>=0){
	  s=printsel(t.sel_row_begin,t.sel_col_begin,t.cur_row,t.cur_col);
	  t.cur_row=t.cmd_row;
	  t.cur_col=t.cmd_col;
	  t.sel_row_begin=-1;
	}
	if (t.cmd_row!=t.cur_row || t.cmd_col!=t.cur_col){
	  s=printcell(t.cur_row,t.cur_col);
	  t.cur_row=t.cmd_row;
	  t.cur_col=t.cmd_col;
	}
	if (s.empty())
	  sheet_cmdline(t,contextptr);
	else {
	  insert(t.cmdline,t.cmd_pos,s.c_str());
	  t.cmd_pos+=s.size();
	}
      } // if t.cmd_row>=0
      else {
	t.cmd_row=t.cur_row;
	t.cmd_col=t.cur_col;
	t.cmd_pos=t.cmdline.size();
      }
      continue;
    case KEY_CTRL_F5: // view
      {
	string value((*t.m[t.cur_row]._VECTptr)[t.cur_col][1].print(contextptr));
	char buf[1024];
	strcpy(buf,value.substr(0,1024-1).c_str());
	textedit(buf,1024-1,contextptr );
      }
      continue;
    case KEY_CHAR_FRAC:
      spread_eval(t.m,contextptr);
      continue;
    case KEY_CTRL_SD: // view graph
      sheet_graph(t,contextptr);
      continue;
    case KEY_CTRL_D: // copy down
      copy_down(t,contextptr);
      continue;
#ifndef NUMWORKS
    case KEY_CTRL_R:
      copy_right(t,contextptr);
      continue;
#endif
    case KEY_CTRL_CATALOG: case KEY_BOOK: case KEY_CTRL_MIXEDFRAC: case '\t':
      {
	if (t.cmd_pos>=0)
	  sheet_help_insert(t,0,contextptr);
      }
      continue;
    } // end switch
    translate_fkey(key);
    if ( (key >= KEY_CTRL_F1 && key <= KEY_CTRL_F3) ||
	  (key >= KEY_CTRL_F7 && key <= KEY_CTRL_F20) 
	 ){
      const char * s=console_menu(key,(unsigned char *)tmenu,0);
      if (s && strlen(s)){
	if (strcmp(s,"undo")==0){
	  t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	  std::swap(t.m,t.undo);
	  sheet_eval(t,contextptr);
	  continue;
	}
	if (strcmp(s,"copy_down")==0){
	  t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	  copy_down(t,contextptr);
	  continue;
	}
	if (strcmp(s,"copy_right")==0){
	  t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	  copy_right(t,contextptr);
	  continue;
	}
	if (strcmp(s,"insert_row")==0){
	  t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	  change_undo(t);
	  t.m=matrice_insert(t.m,t.cur_row,t.cur_col,1,0,makevecteur(0,0,2),contextptr);
	  t.nrows++;
	  continue;
	}
	if (strcmp(s,"insert_col")==0){
	  t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	  change_undo(t);
	  t.m=matrice_insert(t.m,t.cur_row,t.cur_col,0,1,makevecteur(0,0,2),contextptr);
	  t.ncols++;
	  continue;
	}
	if (strcmp(s,"erase_row")==0 && t.nrows>=2){
	  t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	  change_undo(t);
	  t.m=matrice_erase(t.m,t.cur_row,t.cur_col,1,0,contextptr);
	  --t.nrows;
	  continue;
	}
	if (strcmp(s,"erase_col")==0 && t.ncols>=2){
	  t.cmd_pos=t.cmd_row=t.sel_row_begin=-1;
	  change_undo(t);
	  t.m=matrice_erase(t.m,t.cur_row,t.cur_col,0,1,contextptr);
	  --t.ncols;
	  continue;
	}
	if (strcmp(s,"edit_cell")==0){
	  if (t.cmd_row<0 && t.sel_row_begin<0){
	    char buf[1024];
	    strcpy(buf,t.cmdline.substr(0,1024-1).c_str());
	    if (textedit(buf,1024-1,contextptr )){
	      t.cmdline=buf;
	      t.cmd_row=t.cur_row; t.cmd_col=t.cur_col;
	      sheet_cmdline(t,contextptr);
	    }
	  }
	  continue;
	}
	if (t.cmd_row<0)
	  t.cmdline="";
	sheet_cmd(t,s);
      }
      continue;
    }
    if (key==KEY_CHAR_CROCHETS || key==KEY_CHAR_ACCOLADES){
      if (t.cmd_row<0)
	t.cmdline="";
      activate_cmdline(t);
      t.cmdline.insert(t.cmdline.begin()+t.cmd_pos,key==KEY_CHAR_CROCHETS?'[':'{');
      ++t.cmd_pos;
      t.cmdline.insert(t.cmdline.begin()+t.cmd_pos,key==KEY_CHAR_CROCHETS?']':'}');
      continue;
    }
    if (key>=32 && key<128){
      if (t.cmd_row<0)
	t.cmdline="";
      activate_cmdline(t);
      t.cmdline.insert(t.cmdline.begin()+t.cmd_pos,char(key));
      ++t.cmd_pos;
      t.keytooltip=true;
      continue;
    }
    if (const char * ans=keytostring(key,0,false,contextptr)){
      if (ans && strlen(ans)){
	if (t.cmd_row<0)
	  t.cmdline="";
	sheet_cmd(t,ans);
      }
      continue;
    }
    if (key==KEY_CTRL_AC && t.cmd_row>=0){
      if (t.cmdline=="")
	t.cmd_row=-1;
      t.cmdline="";
      t.cmd_pos=0;
      continue;
    }
    
  }
}



// -*- mode:C++ ; compile-command: "g++-3.4 -I. -I.. -g -c Equation.cc -DHAVE_CONFIG_H -DIN_GIAC -Wall" -*-
/*
 *  Copyright (C) 2005,2014 B. Parisse, Institut Fourier, 38402 St Martin d'Heres
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

extern "C" {
#include <fxcg/display.h>
  int RTC_GetTicks();
}
#include "kdisplay.h"
#include "catalogGUI.hpp" // for lang
#include "menuGUI.hpp"
#include "console.h"
#include "textGUI.hpp"
#include "main.h"
#include "graphicsProvider.hpp"
extern giac::context * contextptr;
const int MAX_DISP_RADIUS=2048;

using namespace std;
using namespace giac;

int file_exists(const char * filename){
  if (filename && filename[0]!='\\'){
    // print_msg12("Checking \\\\fls0\\",filename); int key; ck_getkey(&key);
    string file("\\\\fls0\\");
    file += filename;
    return file_exists(file.c_str());
  }
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1);
  int hf = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  // cout << hf << endl << "f:" << filename << endl; Console_Disp();
  if (hf < 0){
    // cout << "file does not exists -2\n";
    return 0; // nothing to load
  }
  Bfile_CloseFile_OS(hf);
  return 1;
}

const char * read_file(const char * filename){
  static string S;
  if (!load_script(filename,S))
    return 0;
  return S.c_str();
}

void os_set_pixel(int x0, int y0,unsigned short color) {
  //if (x0>100 && x0<150 && y0>100 && y0<150) cout << x0 << " " << y0 << '\n';
  //drawRectangle(x0,y0,1,1,color);
  unsigned short* VRAM = VRAM_base;//(unsigned short*)GetVRAMAddress(); 
  VRAM += (y0*LCD_WIDTH_PX + x0);
  *VRAM=color;//COLOR_RED; // color;
}

int os_get_pixel(int x,int y){
  unsigned short* VRAM =(unsigned short*)GetVRAMAddress(); 
  VRAM += (y*LCD_WIDTH_PX + x);
  return *VRAM;
}

int os_draw_string(int X,int Y,int color,int bg,const char * buf,bool fake){
  Y-=24;
  PrintMini(&X, &Y, (unsigned char *)buf, 0, 0xFFFFFFFF, 0, 0, color, bg, fake?0:1, 0);
  return X;
}


int color8(int color){
  double r=(color>>11)/32.;
  double g=((color >> 5) & 0x3f)/64.;
  double b=(color & 0x1f)/32.;
  int index_color=0;
  if (g>=0.5){
    if (r>=0.5){
      if (b>=0.5)
	index_color=7; // white
      else
	index_color=6; // yellow
    }
    else {
      if (b>=0.5)
	index_color=3; // cyan
      else
	index_color=2; // green
    }
  }
  else {
    if (r>=0.5){
      if (b>=0.5)
	index_color=5; // magenta
      else
	index_color=4; // red
    }
    else {
      if (b>=0.5)
	index_color=1; // blue
      else
	index_color=0; // black
    }
  }
}

int os_draw_string_small(int X,int Y,int color,int bg,const char * buf,bool fake){
  // Y-=24; // commented since mode=1<<6
#if 0
  // fake print will find the width of the background rectangle
  int fakeX=X;
  PrintMiniMini( &fakeX, &Y, (unsigned char *)buf, 0, 0, 1 );
  drawRectangle(X,Y,fakeX-X+1,10 /* minimini font height*/,bg);
  // print without +24 (bit 6) and without clearing rectangle (bit 7), done before
  PrintMiniMini( &X, &Y, (unsigned char *)buf, (1<<6) | (1<<7), color8(color), fake?1:0 );
#else
  if (0 && color==0xffff){  // FIXME
    int r=bg>>11,g=(bg>>5)&0x3f,b=bg&0x1f;
    int minic=(r<8?1:0)*4+(g<16?1:0)*2+(b<8?1:0);
    PrintMiniMini(&X,&Y,(unsigned char *)buf,4+(1<<6),minic,fake?1:0);
  }
  else
    PrintMiniMini( &X, &Y, (unsigned char *)buf, (1<<6), color8(color), fake?1:0 );
#endif
  return X;
}
void statuslinemsg(const char * msg){
  DefineStatusMessage((char*)msg, 1, 0, 0);
  DisplayStatusArea();
}
void os_fill_rect(int x,int y,int w,int h,int c){
  drawRectangle(x,y,w,h,c);
}
void os_sync_screen(){
  Bdisp_PutDisp_DD();
}

  const char * python_keywords[] = {   // List of known giac keywords...
    "False",
    "None",
    "True",
    "and",
    "break",
    "continue",
    "def",
    "default",
    "elif",
    "else",
    "except",
    "for",
    "from",
    "global",
    "if",
    "import",
    "not",
    "or",
    "return",
    "try",
    "while",
    "xor",
    "yield",
  };
  const char * const python_builtins[]={
    "NoneType",
    "__call__",
    "__class__",
    "__delitem__",
    "__dir__", 
    "__enter__",
    "__exit__",
    "__getattr__",
    "__getitem__",
    "__hash__",
    "__init__",
    "__int__",
    "__iter__",
    "__len__",
    "__main__",
    "__module__",
    "__name__",
    "__new__",
    "__next__",
    "__qualname__",
    "__repr__",
    "__setitem__",
    "__str__",
    "abs",
    "all",
    "any",
    "append",
    "args",
    "bool",
    "builtins",
    "bytearray",
    "bytecode",
    "bytes",
    "callable",
    "chr",
    "classmethod",
    "clear",
    "close",
    "const",
    "copy",
    "count",
    "dict",
    "dir",
    "divmod",
    "end",
    "endswith",
    "eval",
    "exec",
    "_rm_extend",
    "find",
    "format",
    "from_bytes",
    "get",
    "getattr",
    "globals",
    "hasattr",
    "hash",
    "id",
    "index",
    "insert",
    "int",
    "isalpha",
    "isdigit",
    "isinstance",
    "islower",
    "isspace",
    "issubclass",
    "isupper",
    "items",
    "iter",
    "join",
    "key",
    "keys",
    "len",
    "list",
    "little",
    "locals",
    "lower",
    "lstrip",
    "main",
    "map",
    "micropython",
    "next",
    "object",
    "open",
    "ord",
    "pop",
    "popitem",
    "pow",
    "print",
    "range",
    "read",
    "readinto",
    "readline",
    "remove",
    "replace",
    "repr",
    "reverse",
    "rfind",
    "rindex",
    "round",
    "rsplit",
    "rstrip",
    "self",
    "send",
    "sep",
    "set",
    "setattr",
    "setdefault",
    "sort",
    "sorted",
    "split",
    "start",
    "startswith",
    "staticmethod",
    "step",
    "stop",
    "str",
    "strip",
    "sum",
    "super",
    "throw",
    "to_bytes",
    "tuple",
    "type",
    "update",
    "upper",
    "utf-8",
    "value",
    "values",
    "write",
    "xcas",
    "zip",
  };

  bool is_python_keyword(const char * s){
    return dichotomic_search(python_keywords,sizeof(python_keywords)/sizeof(char*),s)!=-1;
  }
  
  bool is_python_builtin(const char * s){
    return dichotomic_search(python_builtins,sizeof(python_builtins)/sizeof(char*),s)!=-1;
  }

#ifndef NO_NAMESPACE_XCAS
namespace xcas {
#endif // ndef NO_NAMESPACE_XCAS

string print_tableur(const tableur & t,GIAC_CONTEXT){
  string s="spreadsheet[";
  for (int i=0;i<t.nrows;++i){
    printcell_current_row(contextptr)=i;
    s += "[";
    gen g=t.m[i];
    if (g.type!=_VECT) continue;
    vecteur & v=*g._VECTptr;
    for (int j=0;j<t.ncols;++j){
      gen vj=v[j];
      if (vj.type==_VECT && vj._VECTptr->size()==3){
	vecteur vjv=*vj._VECTptr;
	vjv[1]=0;
	vj=gen(vjv,vj.subtype);
      }
      printcell_current_col(contextptr)=j;
      s += vj.print(contextptr);
      if (j==t.ncols-1)
	s += "]";
      else
	s += ",";
    }
    if (i==t.nrows-1)
	s += "]";
    else
      s += ",";      
  }
  return s;
}  

  #ifdef MICROPY_LIB
  void switch_to_micropy(bool chk,GIAC_CONTEXT){
    xcas_python_eval=1;
    int p=python_compat(contextptr);
    if (p<0) p=0;
    python_compat(4|p,contextptr);
    if (edptr)
      edptr->python=1;
    if (chk && do_confirm((lang==1)?"Effacer les variables Xcas?":"Clear Xcas variables?"))
      do_restart();
    cout << "Micropython interpreter\n";
    if (!python_heap){
      python_init(pythonjs_stack_size,pythonjs_heap_size);
      // int key; ck_getkey(&key);
    }
    Console_FMenu_Init();
  }
#endif
  
  unsigned max_prettyprint_equation=512;

  // make a free copy of g
  gen Equation_copy(const gen & g){
    if (g.type==_EQW)
      return *g._EQWptr;
    if (g.type!=_VECT)
      return g;
    vecteur & v = *g._VECTptr;
    const_iterateur it=v.begin(),itend=v.end();
    vecteur res;
    res.reserve(itend-it);
    for (;it!=itend;++it)
      res.push_back(Equation_copy(*it));
    return gen(res,g.subtype);
  }

  // matrix/list select
  bool do_select(gen & eql,bool select,gen & value){
    if (eql.type==_VECT && !eql._VECTptr->empty()){
      vecteur & v=*eql._VECTptr;
      size_t s=v.size();
      if (v[s-1].type!=_EQW)
	return false;
      v[s-1]._EQWptr->selected=select;
      gen sommet=v[s-1]._EQWptr->g;
      --s;
      vecteur args(s);
      for (size_t i=0;i<s;++i){
	if (!do_select(v[i],select,args[i]))
	  return false;
	if (args[i].type==_EQW)
	  args[i]=args[i]._EQWptr->g;
      }
      gen va=s==1?args[0]:gen(args,_SEQ__VECT);
      if (sommet.type==_FUNC)
	va=symbolic(*sommet._FUNCptr,va);
      else
	va=sommet(va,context0);
      //cout << "va " << va << endl;
      value=*v[s]._EQWptr;
      value._EQWptr->g=va;
      //cout << "value " << value << endl;
      return true;
    }
    if (eql.type!=_EQW)
      return false;
    eql._EQWptr->selected=select;
    value=eql;
    return true;
  }
  
  bool Equation_box_sizes(const gen & g,int & l,int & h,int & x,int & y,attributs & attr,bool & selected){
    if (g.type==_EQW){
      eqwdata & w=*g._EQWptr;
      x=w.x;
      y=w.y;
      l=w.dx;
      h=w.dy;
      selected=w.selected;
      attr=w.eqw_attributs;
      //cout << g << endl;
      return true;
    }
    else {
      if (g.type!=_VECT || g._VECTptr->empty() ){
	l=0;
	h=0;
	x=0;
	y=0;
	attr=attributs(0,0,0);
	selected=false;
	return true;
      }
      gen & g1=g._VECTptr->back();
      Equation_box_sizes(g1,l,h,x,y,attr,selected);
      return false;
    }
  }

  // return true if g has some selection inside, gsel points to the selection
  bool Equation_adjust_xy(gen & g,int & xleft,int & ytop,int & xright,int & ybottom,gen * & gsel,gen * & gselparent,int &gselpos,std::vector<int> * goto_ptr){
    gsel=0;
    gselparent=0;
    gselpos=0;
    int x,y,w,h;
    attributs f(0,0,0);
    bool selected;
    Equation_box_sizes(g,w,h,x,y,f,selected);
    if ( (g.type==_EQW__VECT) || selected ){ // terminal or selected
      xleft=x;
      ybottom=y;
      if (selected){ // g is selected
	ytop=y+h;
	xright=x+w;
	gsel =  &g;
	//cout << "adjust " << *gsel << endl;
	return true;
      }
      else { // no selection
	xright=x;
	ytop=y;
	return false;
      }
    }
    if (g.type!=_VECT)
      return false;
    // last not selected, recurse
    iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end()-1;
    for (;it!=itend;++it){
      if (Equation_adjust_xy(*it,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,goto_ptr)){
	if (goto_ptr){
	  goto_ptr->push_back(it-g._VECTptr->begin());
	  //cout << g << ":" << *goto_ptr << endl;
	}
	if (gsel==&*it){
	  // check next siblings
	  
	  gselparent= &g;
	  gselpos=it-g._VECTptr->begin();
	  //cout << "gselparent " << g << endl;
	}
	return true;
      }
    }
    return false;
  }
 
  // select or deselect part of the current eqution
  // This is done *in place*
  void Equation_select(gen & g,bool select){
    if (g.type==_EQW){
      eqwdata & e=*g._EQWptr;
      e.selected=select;
    }
    if (g.type!=_VECT)
      return;
    vecteur & v=*g._VECTptr;
    iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it)
      Equation_select(*it,select);
  }

  // decrease selection (like HP49 eqw Down key)
  int eqw_select_down(gen & g){
    int xleft,ytop,xright,ybottom,gselpos;
    int newxleft,newytop,newxright,newybottom;
    gen * gsel,*gselparent;
    if (Equation_adjust_xy(g,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos)){
      //cout << "select down before " << *gsel << endl;
      if (gsel->type==_VECT && !gsel->_VECTptr->empty()){
	Equation_select(*gsel,false);
	Equation_select(gsel->_VECTptr->front(),true);
	//cout << "select down after " << *gsel << endl;
	Equation_adjust_xy(g,newxleft,newytop,newxright,newybottom,gsel,gselparent,gselpos);
	return newytop-ytop;
      }
    }
    return 0;
  }

  int eqw_select_up(gen & g){
    int xleft,ytop,xright,ybottom,gselpos;
    int newxleft,newytop,newxright,newybottom;
    gen * gsel,*gselparent;
    if (Equation_adjust_xy(g,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos) && gselparent){
      Equation_select(*gselparent,true);
      //cout << "gselparent " << *gselparent << endl;
      Equation_adjust_xy(g,newxleft,newytop,newxright,newybottom,gsel,gselparent,gselpos);
      return newytop-ytop;
    }
    return false;
  }

  // exchange==0 move selection to left or right sibling, ==2 add left or right
  // sibling, ==1 exchange selection with left or right sibling
  int eqw_select_leftright(Equation & eq,bool left,int exchange){
    gen & g=eq.data;
    int xleft,ytop,xright,ybottom,gselpos;
    int newxleft,newytop,newxright,newybottom;
    gen * gsel,*gselparent;
    vector<int> goto_sel;
    if (Equation_adjust_xy(g,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gselparent && gselparent->type==_VECT){
      vecteur & gselv=*gselparent->_VECTptr;
      int n=gselv.size()-1,gselpos_orig=gselpos;
      if (n<1) return 0;
      if (left) {
	if (gselpos==0)
	  gselpos=n-1;
	else
	  gselpos--;
      }
      else {
	if (gselpos==n-1)
	  gselpos=0;
	else
	  gselpos++;
      }
      if (exchange==1){ // exchange gselpos_orig and gselpos
	swapgen(gselv[gselpos],gselv[gselpos_orig]);
	gsel=&gselv[gselpos_orig];
	gen value;
	if (xcas::do_select(*gsel,true,value) && value.type==_EQW)
	  replace_selection(eq,value._EQWptr->g,gsel,&goto_sel);
      }
      else {
	// increase selection to next sibling possible for + and * only
	if (n>2 && exchange==2 && gselv[n].type==_EQW && (gselv[n]._EQWptr->g==at_plus || gselv[n]._EQWptr->g==at_prod)){
	  gen value1, value2,tmp;
	  if (gselpos_orig<gselpos)
	    swapint(gselpos_orig,gselpos);
	  // now gselpos<gselpos_orig
	  xcas::do_select(gselv[gselpos_orig],true,value1);
	  xcas::do_select(gselv[gselpos],true,value2);
	  if (value1.type==_EQW && value2.type==_EQW){
	    tmp=gselv[n]._EQWptr->g==at_plus?value1._EQWptr->g+value2._EQWptr->g:value1._EQWptr->g*value2._EQWptr->g;
	    gselv.erase(gselv.begin()+gselpos_orig);
	    replace_selection(eq,tmp,&gselv[gselpos],&goto_sel);
	  }
	}
	else {
	  Equation_select(*gselparent,false);
	  gen & tmp=(*gselparent->_VECTptr)[gselpos];
	  Equation_select(tmp,true);
	}
      }
      Equation_adjust_xy(g,newxleft,newytop,newxright,newybottom,gsel,gselparent,gselpos);
      return newxleft-xleft;
    }
    return 0;
  }

  bool eqw_select(const gen & eq,int l,int c,bool select,gen & value){
    value=undef;
    if (l<0 || eq.type!=_VECT || eq._VECTptr->size()<=l)
      return false;
    gen & eql=(*eq._VECTptr)[l];
    if (c<0)
      return do_select(eql,select,value);
    if (eql.type!=_VECT || eql._VECTptr->size()<=c)
      return false;
    gen & eqlc=(*eql._VECTptr)[c];
    return do_select(eqlc,select,value);
  }

  gen Equation_compute_size(const gen & g,const attributs & a,int windowhsize,GIAC_CONTEXT);
  
  // void Bdisp_MMPrint(int x, int y, const char* string, int mode_flags, int xlimit, int P6, int P7, int color, int back_color, int writeflag, int P11); 
  // void PrintCXY(int x, int y, const char *cptr, int mode_flags, int P5, int color, int back_color, int P8, int P9)
  // void PrintMini( int* x, int* y, const char* string, int mode_flags, unsigned int xlimit, int P6, int P7, int color, int back_color, int writeflag, int P11)

  void text_print(int fontsize,const char * s,int x,int y,int c=COLOR_BLACK,int bg=COLOR_WHITE,int mode=0){
    // *logptr(contextptr) << x << " " << y << " " << fontsize << " " << s << endl; return;
    c=(unsigned short) c;
    if (x>LCD_WIDTH_PX) return;
    int ss=strlen(s);
    if (ss==1 && s[0]==0x1e){ // arrow for limit
      if (mode==4)
	c=bg;
      draw_line(x,y-4,x+fontsize/2,y-4,c);
      draw_line(x,y-3,x+fontsize/2,y-3,c);
      draw_line(x+fontsize/2-4,y,x+fontsize/2,y-4,c);
      draw_line(x+fontsize/2-3,y,x+fontsize/2+1,y-4,c);
      draw_line(x+fontsize/2-4,y-7,x+fontsize/2,y-3,c);   
      draw_line(x+fontsize/2-3,y-7,x+fontsize/2+1,y-3,c);   
      return;
    }
    if (ss==2 && strcmp(s,"pi")==0){
      if (mode==4){
	drawRectangle(x,y+2-fontsize,fontsize,fontsize,c);
	c=bg;
      }
      draw_line(x+fontsize/3-1,y+1,x+fontsize/3,y+6-fontsize,c);
      draw_line(x+fontsize/3-2,y+1,x+fontsize/3-1,y+6-fontsize,c);
      draw_line(x+2*fontsize/3,y+1,x+2*fontsize/3,y+6-fontsize,c);
      draw_line(x+2*fontsize/3+1,y+1,x+2*fontsize/3+1,y+6-fontsize,c);
      draw_line(x+2,y+6-fontsize,x+fontsize,y+6-fontsize,c);
      draw_line(x+2,y+5-fontsize,x+fontsize,y+5-fontsize,c);
      return;
    }
    if (fontsize>=16 && ss==2 && s[0]==char(0xe5) && (s[1]==char(0xea) || s[1]==char(0xeb))) // special handling for increasing and decreasing in tabvar output
      fontsize=18;
    if (fontsize>=18){
      y -= 40;//36; // status area shift
      PrintMini(&x,&y,(unsigned char *)s,mode,0xffffffff,0,0,c,bg,1,0);
      return;
    }
    if (fontsize<16){
      y -= 36;//32;
      PrintMiniMini( &x, &y, (unsigned char *)s, mode,c, 0 );
      return;
    }
    y -= 38;//34;
    //PrintCXY(x,y,s,TEXT_MODE_NORMAL,-1,COLOR_BLACK,COLOR_WHITE,1,0);
    Bdisp_MMPrint(x,y,s,mode,0xffffffff,0,0,c,bg,1,0);
  }

  
  
  int text_width(int fontsize,const char * s){
    if (fontsize>=18)
      return os_draw_string(0,0,_WHITE,_BLACK,s,true);
    if (fontsize<16)
      return os_draw_string_small(0,0,_WHITE,_BLACK,s,true);
    return os_draw_string(0,0,_WHITE,_BLACK,s,true)*7.4/8;
  }

  int fl_width(const char * s){ // FIXME do a fake print
    return text_width(14,s);
  }

  void fl_arc(int x,int y,int rx,int ry,int theta1_deg,int theta2_deg,int c=COLOR_BLACK){
    rx/=2;
    ry/=2;
    // *logptr(contextptr) << "theta " << theta1_deg << " " << theta2_deg << endl;
    if (ry==rx){
      if (theta2_deg-theta1_deg==360){
	draw_circle(x+rx,y+rx,rx,c,true,true,true,true);
	return;
      }
      if (theta1_deg==0 && theta2_deg==180){
	draw_circle(x+rx,y+rx,rx,c,true,true,false,false);
	return;
      }
      if (theta1_deg==180 && theta2_deg==360){
	draw_circle(x+rx,y+rx,rx,c,false,false,true,true);
	return;
      }
    }
    // *logptr(contextptr) << "draw_arc" << theta1_deg*M_PI/180. << " " << theta2_deg*M_PI/180. << endl;
    draw_arc(x+rx,y+ry,rx,ry,c,theta1_deg*M_PI/180.,theta2_deg*M_PI/180.);
  }

  void fl_pie(int x,int y,int rx,int ry,int theta1_deg,int theta2_deg,int c=COLOR_BLACK,bool segment=false){
    //cout << "fl_pie r=" << rx << " " << ry << " t=" << theta1_deg << " " << theta2_deg << " " << c << endl;
    if (!segment && ry==rx){
      if (theta2_deg-theta1_deg>=360){
	rx/=2;
	draw_filled_circle(x+rx,y+rx,rx,c);
	return;
      }
      if (theta1_deg==-90 && theta2_deg==90){
	rx/=2;
	draw_filled_circle(x+rx,y+rx,rx,c,false,true);
	return;
      }
      if (theta1_deg==90 && theta2_deg==270){
	rx/=2;
	draw_filled_circle(x+rx,y+rx,rx,c,true,false);
	return;
      }
    }
    // approximation by a filled polygon
    // points: (x,y), (x+rx*cos(theta)/2,y+ry*sin(theta)/2) theta=theta1..theta2
    while (theta2_deg<theta1_deg)
      theta2_deg+=360;
    if (theta2_deg-theta1_deg>=360){
      theta1_deg=0;
      theta2_deg=360;
    }
    int N0=theta2_deg-theta1_deg+1;
    // reduce N if rx or ry is small
    double red=double(rx)/LCD_WIDTH_PX*double(ry)/LCD_HEIGHT_PX;
    if (red>1) red=1;
    if (red<0.1) red=0.1;
    int N=red*N0;
    if (N<5)
      N=N0>5?5:N0;
    if (N<2)
      N=2;
    vector< vector<int> > v(segment?N+1:N+2,vector<int>(2));
    x += rx/2;
    y += ry/2;
    int i=0;
    if (!segment){
      v[0][0]=x;
      v[0][1]=y;
      ++i;
    }
    double theta=theta1_deg*M_PI/180;
    double thetastep=(theta2_deg-theta1_deg)*M_PI/(180*(N-1));
    for (;i<v.size()-1;++i){
      v[i][0]=int(x+rx*std::cos(theta)/2+.5);
      v[i][1]=int(y-ry*std::sin(theta)/2+.5); // y is inverted
      theta += thetastep;
    }
    v.back()=v.front();
    draw_filled_polygon(v,0,LCD_WIDTH_PX,24,LCD_HEIGHT_PX,c);
  }

  bool binary_op(const unary_function_ptr & u){
    const unary_function_ptr binary_op_tab_ptr []={*at_plus,*at_prod,*at_pow,*at_and,*at_ou,*at_xor,*at_different,*at_same,*at_equal,*at_unit,*at_compose,*at_composepow,*at_deuxpoints,*at_tilocal,*at_pointprod,*at_pointdivision,*at_pointpow,*at_division,*at_normalmod,*at_minus,*at_intersect,*at_union,*at_interval,*at_inferieur_egal,*at_inferieur_strict,*at_superieur_egal,*at_superieur_strict,*at_equal2,0};
    return equalposcomp(binary_op_tab_ptr,u);
  }
  
  eqwdata Equation_total_size(const gen & g){
    if (g.type==_EQW)
      return *g._EQWptr;
    if (g.type!=_VECT || g._VECTptr->empty())
      return eqwdata(0,0,0,0,attributs(0,0,0),undef);
    return Equation_total_size(g._VECTptr->back());
  }

  // find smallest value of y and height
  void Equation_y_dy(const gen & g,int & y,int & dy){
    y=0; dy=0;
    if (g.type==_EQW){
      y=g._EQWptr->y;
      dy=g._EQWptr->dy;
    }
    if (g.type==_VECT){
      iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end();
      for (;it!=itend;++it){
	int Y,dY;
	Equation_y_dy(*it,Y,dY);
	// Y, Y+dY and y,y+dy
	int ymax=giacmax(y+dy,Y+dY);
	if (Y<y)
	  y=Y;
	dy=ymax-y;
      }
    }
  }

  void Equation_translate(gen & g,int deltax,int deltay){
    if (g.type==_EQW){
      g._EQWptr->x += deltax;
      g._EQWptr->y += deltay;
      g._EQWptr->baseline += deltay;
      return ;
    }
    if (g.type!=_VECT)
      setsizeerr();
    vecteur & v=*g._VECTptr;
    iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it)
      Equation_translate(*it,deltax,deltay);
  }

  gen Equation_change_attributs(const gen & g,const attributs & newa){
    if (g.type==_EQW){
      gen res(*g._EQWptr);
      res._EQWptr->eqw_attributs = newa;
      return res;
    }
    if (g.type!=_VECT)
      return gensizeerr();
    vecteur v=*g._VECTptr;
    iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it)
      *it=Equation_change_attributs(*it,newa);
    return gen(v,g.subtype);
  }

  vecteur Equation_subsizes(const gen & arg,const attributs & a,int windowhsize,GIAC_CONTEXT){
    vecteur v;
    if ( (arg.type==_VECT) && ( (arg.subtype==_SEQ__VECT) 
				// || (!ckmatrix(arg)) 
				) ){
      const_iterateur it=arg._VECTptr->begin(),itend=arg._VECTptr->end();
      for (;it!=itend;++it)
	v.push_back(Equation_compute_size(*it,a,windowhsize,contextptr));
    }
    else {
      v.push_back(Equation_compute_size(arg,a,windowhsize,contextptr));
    }
    return v;
  }

  // vertical merge with same baseline
  // for vertical merge of hp,yp at top (like ^) add fontsize to yp
  // at bottom (like lower bound of int) substract fontsize from yp
  void Equation_vertical_adjust(int hp,int yp,int & h,int & y){
    int yf=min(y,yp);
    h=max(y+h,yp+hp)-yf;
    y=yf;
  }

  gen Equation_compute_symb_size(const gen & g,const attributs & a,int windowhsize,GIAC_CONTEXT){
    if (g.type!=_SYMB)
      return Equation_compute_size(g,a,windowhsize,contextptr);
    unary_function_ptr & u=g._SYMBptr->sommet;
    gen arg=g._SYMBptr->feuille,rootof_value;
    if (u==at_makevector){
      vecteur v(1,arg);
      if (arg.type==_VECT)
	v=*arg._VECTptr;
      iterateur it=v.begin(),itend=v.end();
      for (;it!=itend;++it){
	if ( (it->type==_SYMB) && (it->_SYMBptr->sommet==at_makevector) )
	  *it=_makevector(it->_SYMBptr->feuille,contextptr);
      }
      return Equation_compute_size(v,a,windowhsize,contextptr);
    }
    if (u==at_makesuite){
      if (arg.type==_VECT)
	return Equation_compute_size(gen(*arg._VECTptr,_SEQ__VECT),a,windowhsize,contextptr);
      else
	return Equation_compute_size(arg,a,windowhsize,contextptr);
    }
    if (u==at_sqrt)
      return Equation_compute_size(symb_pow(arg,plus_one_half),a,windowhsize,contextptr);
    if (u==at_division){
      if (arg.type!=_VECT || arg._VECTptr->size()!=2)
	return Equation_compute_size(arg,a,windowhsize,contextptr);
      gen tmp;
      tmp.__FRACptr = new ref_fraction(Tfraction<gen>(arg._VECTptr->front(),arg._VECTptr->back()));
      tmp.type=_FRAC;
      return Equation_compute_size(tmp,a,windowhsize,contextptr);
    }
    if (u==at_prod){
      gen n,d;
      if (rewrite_prod_inv(arg,n,d)){
	if (n.is_symb_of_sommet(at_neg))
	  return Equation_compute_size(symb_neg(Tfraction<gen>(-n,d)),a,windowhsize,contextptr);
	return Equation_compute_size(Tfraction<gen>(n,d),a,windowhsize,contextptr);
      }
    }
    if (u==at_inv){
      if ( (is_integer(arg) && is_positive(-arg,contextptr))
	   || (arg.is_symb_of_sommet(at_neg)))
	return Equation_compute_size(symb_neg(Tfraction<gen>(plus_one,-arg)),a,windowhsize,contextptr);
      return Equation_compute_size(Tfraction<gen>(plus_one,arg),a,windowhsize,contextptr);
    }
    if (u==at_expr && arg.type==_VECT && arg.subtype==_SEQ__VECT && arg._VECTptr->size()==2 && arg._VECTptr->back().type==_INT_){
      gen varg1=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv(Equation_total_size(varg1));
      gen varg2=eqwdata(0,0,0,0,a,arg._VECTptr->back());
      vecteur v12(makevecteur(varg1,varg2));
      v12.push_back(eqwdata(vv.dx,vv.dy,0,vv.y,a,at_expr,0));
      return gen(v12,_SEQ__VECT);
    }
    int llp=int(text_width(a.fontsize,("(")))-1;
    int lrp=llp;
    int lc=int(text_width(a.fontsize,(",")));
    string us=u.ptr()->s;
    int ls=int(text_width(a.fontsize,(us.c_str())));
    // if (isalpha(u.ptr()->s[0])) ls += 1;
    if (u==at_abs)
      ls = 2;
    // special cases first int, sigma, /, ^
    // and if printed as printsommetasoperator
    // otherwise print with usual functional notation
    int x=0;
    int h=a.fontsize;
    int y=0;
#if 1
    if ((u==at_integrate) || (u==at_sum) ){ // Int
      int s=1;
      if (arg.type==_VECT)
	s=arg._VECTptr->size();
      else
	arg=vecteur(1,arg);
      // s==1 -> general case
      if ( (s==1) || (s==2) ){ // int f(x) dx and sum f(n) n
	vecteur v(Equation_subsizes(gen(*arg._VECTptr,_SEQ__VECT),a,windowhsize,contextptr));
	eqwdata vv(Equation_total_size(v[0]));
	if (s==1){
	  x=a.fontsize;
	  Equation_translate(v[0],x,0);
	  x += int(text_width(a.fontsize,(" dx")));
	}
	if (s==2){
	  if (u==at_integrate){
	    x=a.fontsize;
	    Equation_translate(v[0],x,0);
	    x += vv.dx+int(text_width(a.fontsize,(" d")));
	    Equation_vertical_adjust(vv.dy,vv.y,h,y);
	    vv=Equation_total_size(v[1]);
	    Equation_translate(v[1],x,0);
	    Equation_vertical_adjust(vv.dy,vv.y,h,y);
	  }
	  else {
	    Equation_vertical_adjust(vv.dy,vv.y,h,y);
	    eqwdata v1=Equation_total_size(v[1]);
	    x=max(a.fontsize,v1.dx)+2*a.fontsize/3; // var name size
	    Equation_translate(v[1],0,-v1.dy-v1.y);
	    Equation_vertical_adjust(v1.dy,-v1.dy,h,y);
	    Equation_translate(v[0],x,0);
	    x += vv.dx; // add function size
	  }
	}
	if (u==at_integrate){
	  x += vv.dx;
	  if (h==a.fontsize)
	    h+=2*a.fontsize/3;
	  if (y==0){
	    y=-2*a.fontsize/3;
	    h+=2*a.fontsize/3;
	  }
	}
	v.push_back(eqwdata(x,h,0,y,a,u,0));
	return gen(v,_SEQ__VECT);
      }
      if (s>=3){ // int _a^b f(x) dx
	vecteur & intarg=*arg._VECTptr;
	gen tmp_l,tmp_u,tmp_f,tmp_x;
	attributs aa(a);
	if (a.fontsize>=10)
	  aa.fontsize -= 2;
	tmp_f=Equation_compute_size(intarg[0],a,windowhsize,contextptr);
	tmp_x=Equation_compute_size(intarg[1],a,windowhsize,contextptr);
	tmp_l=Equation_compute_size(intarg[2],aa,windowhsize,contextptr);
	if (s==4)
	  tmp_u=Equation_compute_size(intarg[3],aa,windowhsize,contextptr);
	x=a.fontsize+(u==at_integrate?-2:+4);
	eqwdata vv(Equation_total_size(tmp_l));
	Equation_translate(tmp_l,x,-vv.y-vv.dy);
	vv=Equation_total_size(tmp_l);
	Equation_vertical_adjust(vv.dy,vv.y,h,y);
	int lx = vv.dx;
	if (s==4){
	  vv=Equation_total_size(tmp_u);
	  Equation_translate(tmp_u,x,a.fontsize-3-vv.y);
	  vv=Equation_total_size(tmp_u);
	  Equation_vertical_adjust(vv.dy,vv.y,h,y);
	}
	x += max(lx,vv.dx);
	Equation_translate(tmp_f,x,0);
	vv=Equation_total_size(tmp_f);
	Equation_vertical_adjust(vv.dy,vv.y,h,y);
	if (u==at_integrate){
	  x += vv.dx+int(text_width(a.fontsize,(" d")));
	  Equation_translate(tmp_x,x,0);
	  vv=Equation_total_size(tmp_x);
	  Equation_vertical_adjust(vv.dy,vv.y,h,y);
	  x += vv.dx;
	}
	else {
	  x += vv.dx;
	  Equation_vertical_adjust(vv.dy,vv.y,h,y);
	  vv=Equation_total_size(tmp_x);
	  x=max(x,vv.dx)+a.fontsize/3;
	  Equation_translate(tmp_x,0,-vv.dy-vv.y);
	  //Equation_translate(tmp_l,0,-1);	  
	  if (s==4) Equation_translate(tmp_u,-2,0);	  
	  Equation_vertical_adjust(vv.dy,-vv.dy,h,y);
	}
	vecteur res(makevecteur(tmp_f,tmp_x,tmp_l));
	if (s==4)
	  res.push_back(tmp_u);
	res.push_back(eqwdata(x,h,0,y,a,u,0));
	return gen(res,_SEQ__VECT);
      }
    }
    if (u==at_limit && arg.type==_VECT){ // limit
      vecteur limarg=*arg._VECTptr;
      int s=limarg.size();
      if (s==2 && limarg[1].is_symb_of_sommet(at_equal)){
	limarg.push_back(limarg[1]._SYMBptr->feuille[1]);
	limarg[1]=limarg[1]._SYMBptr->feuille[0];
	++s;
      }
      if (s>=3){
	gen tmp_l,tmp_f,tmp_x,tmp_dir;
	attributs aa(a);
	if (a.fontsize>=10)
	  aa.fontsize -= 2;
	tmp_f=Equation_compute_size(limarg[0],a,windowhsize,contextptr);
	tmp_x=Equation_compute_size(limarg[1],aa,windowhsize,contextptr);
	tmp_l=Equation_compute_size(limarg[2],aa,windowhsize,contextptr);
	if (s==4)
	  tmp_dir=Equation_compute_size(limarg[3],aa,windowhsize,contextptr);
	eqwdata vf(Equation_total_size(tmp_f));
	eqwdata vx(Equation_total_size(tmp_x));
	eqwdata vl(Equation_total_size(tmp_l));
	eqwdata vdir(Equation_total_size(tmp_dir));
	int sous=max(vx.dy,vl.dy);
	if (s==4)
	  Equation_translate(tmp_f,vx.dx+vl.dx+vdir.dx+a.fontsize+4,0);
	else
	  Equation_translate(tmp_f,vx.dx+vl.dx+a.fontsize+2,0);
	Equation_translate(tmp_x,0,-sous-vl.y);
	Equation_translate(tmp_l,vx.dx+a.fontsize+2,-sous-vl.y);
	if (s==4)
	  Equation_translate(tmp_dir,vx.dx+vl.dx+a.fontsize+4,-sous-vl.y);
	h=vf.dy;
	y=vf.y;
	vl=Equation_total_size(tmp_l);
	Equation_vertical_adjust(vl.dy,vl.y,h,y);
	vecteur res(makevecteur(tmp_f,tmp_x,tmp_l));
	if (s==4){
	  res.push_back(tmp_dir);
	  res.push_back(eqwdata(vf.dx+vx.dx+a.fontsize+4+vl.dx+vdir.dx,h,0,y,a,u,0));
	}
	else
	  res.push_back(eqwdata(vf.dx+vx.dx+a.fontsize+2+vl.dx,h,0,y,a,u,0));
	return gen(res,_SEQ__VECT);
      }
    }
#endif
    if ( (u==at_of || u==at_at) && arg.type==_VECT && arg._VECTptr->size()==2 ){
      // user function, function in 1st arg, arguments in 2nd arg
      gen varg1=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg1);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      gen arg2=arg._VECTptr->back();
      if (u==at_at && xcas_mode(contextptr)!=0){
	if (arg2.type==_VECT)
	  arg2=gen(addvecteur(*arg2._VECTptr,vecteur(arg2._VECTptr->size(),plus_one)),_SEQ__VECT);
	else
	  arg2=arg2+plus_one; 
      }
      gen varg2=Equation_compute_size(arg2,a,windowhsize,contextptr);
      Equation_translate(varg2,vv.dx+llp,0);
      vv=Equation_total_size(varg2);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      vecteur res(makevecteur(varg1,varg2));
      res.push_back(eqwdata(vv.dx+vv.x+lrp,h,0,y,a,u,0));
      return gen(res,_SEQ__VECT);
    }
    if (u==at_pow){ 
      // first arg not translated
      gen varg=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      // 1/2 ->sqrt, otherwise as exponent
      if (arg._VECTptr->back()==plus_one_half){
	Equation_translate(varg,a.fontsize,0);
	vecteur res(1,varg);
	res.push_back(eqwdata(vv.dx+a.fontsize,vv.dy+4,vv.x,vv.y,a,at_sqrt,0));
	return gen(res,_SEQ__VECT);
      }
      bool needpar=vv.g.type==_FUNC || vv.g.is_symb_of_sommet(at_pow) || need_parenthesis(vv.g);
      if (needpar)
	x=llp;
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      vecteur res(1,varg);
      // 2nd arg translated 
      if (needpar)
	x+=vv.dx+lrp;
      else
	x+=vv.dx+1;
      int arg1dy=vv.dy,arg1y=vv.y;
      if (a.fontsize>=16){
	attributs aa(a);
	aa.fontsize -= 2;
	varg=Equation_compute_size(arg._VECTptr->back(),aa,windowhsize,contextptr);
      }
      else
	varg=Equation_compute_size(arg._VECTptr->back(),a,windowhsize,contextptr);
      vv=Equation_total_size(varg);
      Equation_translate(varg,x,arg1y+(3*arg1dy)/4-vv.y);
      res.push_back(varg);
      vv=Equation_total_size(varg);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      x += vv.dx;
      res.push_back(eqwdata(x,h,0,y,a,u,0));
      return gen(res,_SEQ__VECT);
    }
    if (u==at_factorial){
      vecteur v;
      gen varg=Equation_compute_size(arg,a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      bool paren=need_parenthesis(vv.g) || vv.g==at_prod || vv.g==at_division || vv.g==at_pow;
      if (paren)
	x+=llp;
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      v.push_back(varg);
      x += vv.dx;
      if (paren)
	x+=lrp;
      varg=eqwdata(x+4,h,0,y,a,u,0);
      v.push_back(varg);
      return gen(v,_SEQ__VECT);
    }
    if (u==at_sto){ // A:=B, *it -> B
      gen varg=Equation_compute_size(arg._VECTptr->back(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      Equation_translate(varg,x,0);
      vecteur v(2);
      v[1]=varg;
      x+=vv.dx;
      x+=ls+3;
      // first arg not translated
      varg=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      vv=Equation_total_size(varg);
      if (need_parenthesis(vv.g))
	x+=llp;
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      v[0]=varg;
      x += vv.dx;
      if (need_parenthesis(vv.g))
	x+=lrp;
      v.push_back(eqwdata(x,h,0,y,a,u,0));
      return gen(v,_SEQ__VECT);
    }
    if (u==at_program && arg._VECTptr->back().type!=_VECT && !arg._VECTptr->back().is_symb_of_sommet(at_local) ){
      gen varg=Equation_compute_size(arg._VECTptr->front(),a,windowhsize,contextptr);
      eqwdata vv=Equation_total_size(varg);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      Equation_translate(varg,x,0);
      vecteur v(2);
      v[0]=varg;
      x+=vv.dx;
      x+=int(text_width(a.fontsize,("->")))+3;
      varg=Equation_compute_size(arg._VECTptr->back(),a,windowhsize,contextptr);
      vv=Equation_total_size(varg);
      Equation_translate(varg,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      v[1]=varg;
      x += vv.dx;
      v.push_back(eqwdata(x,h,0,y,a,u,0));
      return gen(v,_SEQ__VECT);      
    }
    bool binaryop= (u.ptr()->printsommet==&printsommetasoperator) || binary_op(u);
    if ( u!=at_sto && u.ptr()->printsommet!=NULL && !binaryop ){
      gen tmp=string2gen(g.print(contextptr),false);
      return Equation_compute_size(symbolic(at_expr,makesequence(tmp,xcas_mode(contextptr))),a,windowhsize,contextptr);
    }
    vecteur v;
    if (!binaryop || arg.type!=_VECT)
      v=Equation_subsizes(arg,a,windowhsize,contextptr);
    else
      v=Equation_subsizes(gen(*arg._VECTptr,_SEQ__VECT),a,windowhsize,contextptr);
    iterateur it=v.begin(),itend=v.end();
    if ( it==itend || (itend-it==1) ){ 
      gen gtmp;
      if (it==itend)
	gtmp=Equation_compute_size(gen(vecteur(0),_SEQ__VECT),a,windowhsize,contextptr);
      else
	gtmp=*it;
      // unary op, shift arg position horizontally
      eqwdata vv=Equation_total_size(gtmp);
      bool paren = u!=at_neg || (vv.g!=at_prod && need_parenthesis(vv.g)) ;
      x=ls+(paren?llp:0);
      gen tmp=gtmp; Equation_translate(tmp,x,0);
      x=x+vv.dx+(paren?lrp:0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      return gen(makevecteur(tmp,eqwdata(x,h,0,y,a,u,0)),_EQW__VECT);
    }
    if (binaryop){ // op (default with par)
      int currenth=h,largeur=0;
      iterateur itprec=v.begin();
      h=0;
      if (u==at_plus){ // op without parenthesis
	if (it->type==_VECT && !it->_VECTptr->empty() && it->_VECTptr->back().type==_EQW && it->_VECTptr->back()._EQWptr->g==at_equal)
	  ;
	else {
	  llp=0;
	  lrp=0;
	}
      }
      for (;;){
	eqwdata vv=Equation_total_size(*it);
	if (need_parenthesis(vv.g))
	  x+=llp;
	if (u==at_plus && it!=v.begin() &&
	    ( 
	     (it->type==_VECT && it->_VECTptr->back().type==_EQW && it->_VECTptr->back()._EQWptr->g==at_neg) 
	     || 
	     ( it->type==_EQW && (is_integer(it->_EQWptr->g) || it->_EQWptr->g.type==_DOUBLE_) && is_strictly_positive(-it->_EQWptr->g,contextptr) ) 
	      ) 
	    )
	  x -= ls;
#if 0 //
	if (x>windowhsize-vv.dx && x>windowhsize/2 && (itend-it)*vv.dx>windowhsize/2){
	  largeur=max(x,largeur);
	  x=0;
	  if (need_parenthesis(vv.g))
	    x+=llp;
	  h+=currenth;
	  Equation_translate(*it,x,0);
	  for (iterateur kt=v.begin();kt!=itprec;++kt)
	    Equation_translate(*kt,0,currenth);
	  if (y){
	    for (iterateur kt=itprec;kt!=it;++kt)
	      Equation_translate(*kt,0,-y);
	  }
	  itprec=it;
	  currenth=vv.dy;
	  y=vv.y;
	}
	else
#endif
	  {
	    Equation_translate(*it,x,0);
	    vv=Equation_total_size(*it);
	    Equation_vertical_adjust(vv.dy,vv.y,currenth,y);
	  }
	x+=vv.dx;
	if (need_parenthesis(vv.g))
	  x+=lrp;
	++it;
	if (it==itend){
	  for (iterateur kt=v.begin();kt!=itprec;++kt)
	    Equation_translate(*kt,0,currenth+y);
	  h+=currenth;
	  v.push_back(eqwdata(max(x,largeur),h,0,y,a,u,0));
	  //cout << v << endl;
	  return gen(v,_SEQ__VECT);
	}
	x += ls+3;
      } 
    }
    // normal printing
    x=ls+llp;
    for (;;){
      eqwdata vv=Equation_total_size(*it);
      Equation_translate(*it,x,0);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      x+=vv.dx;
      ++it;
      if (it==itend){
	x+=lrp;
	v.push_back(eqwdata(x,h,0,y,a,u,0));
	return gen(v,_SEQ__VECT);
      }
      x+=lc;
    }
  }

  // windowhsize is used for g of type HIST__VECT (history) right justify answers
  // Returns either a eqwdata type object (terminal) or a vector 
  // (of subtype _EQW__VECT or _HIST__VECT)
  gen Equation_compute_size(const gen & g,const attributs & a,int windowhsize,GIAC_CONTEXT){
    /*****************
     *   FRACTIONS   *
     *****************/
    if (g.type==_FRAC){
      if (is_integer(g._FRACptr->num) && is_positive(-g._FRACptr->num,contextptr))
	return Equation_compute_size(symb_neg(fraction(-g._FRACptr->num,g._FRACptr->den)),a,windowhsize,contextptr);
      gen v1=Equation_compute_size(g._FRACptr->num,a,windowhsize,contextptr);
      eqwdata vv1=Equation_total_size(v1);
      gen v2=Equation_compute_size(g._FRACptr->den,a,windowhsize,contextptr);
      eqwdata vv2=Equation_total_size(v2);
      // Center the fraction
      int w1=vv1.dx,w2=vv2.dx;
      int w=max(w1,w2)+6;
      vecteur v(3);
      v[0]=v1; Equation_translate(v[0],(w-w1)/2,11-vv1.y);
      v[1]=v2; Equation_translate(v[1],(w-w2)/2,5-vv2.dy-vv2.y);
      v[2]=eqwdata(w,a.fontsize/2+vv1.dy+vv2.dy+1,0,(a.fontsize<=14?4:3)-vv2.dy,a,at_division,0);
      return gen(v,_SEQ__VECT);
    }
    /***************
     *   VECTORS   *
     ***************/
    if ( (g.type==_VECT) && !g._VECTptr->empty() ){
      vecteur v;
      const_iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end();
      int x=0,y=0,h=a.fontsize; 
      /***************
       *   MATRICE   *
       ***************/
      bool gmat=ckmatrix(g);
      vector<int> V; int p=0;
      if (!gmat && is_mod_vecteur(*g._VECTptr,V,p) && p!=0){
	gen gm=makemodquoted(unmod(g),p);
	return Equation_compute_size(gm,a,windowhsize,contextptr);
      }
      vector< vector<int> > M; 
      if (gmat && is_mod_matrice(*g._VECTptr,M,p) && p!=0){
	gen gm=makemodquoted(unmod(g),p);
	return Equation_compute_size(gm,a,windowhsize,contextptr);
      }
      if (gmat && g.subtype!=_SEQ__VECT && g.subtype!=_SET__VECT && g.subtype!=_POLY1__VECT && g._VECTptr->front().subtype!=_SEQ__VECT){
	gen mkvect(at_makevector);
	mkvect.subtype=_SEQ__VECT;
	gen mkmat(at_makevector);
	mkmat.subtype=_MATRIX__VECT;
	int nrows,ncols;
	mdims(*g._VECTptr,nrows,ncols);
	if (ncols){
	  vecteur all_sizes;
	  all_sizes.reserve(nrows);
	  vector<int> row_heights(nrows),row_bases(nrows),col_widths(ncols);
	  // vertical gluing
	  for (int i=0;it!=itend;++it,++i){
	    gen tmpg=*it;
	    tmpg.subtype=_SEQ__VECT;
	    vecteur tmp(Equation_subsizes(tmpg,a,max(windowhsize/ncols-a.fontsize,230),contextptr));
	    int h=a.fontsize,y=0;
	    const_iterateur jt=tmp.begin(),jtend=tmp.end();
	    for (int j=0;jt!=jtend;++jt,++j){
	      eqwdata w(Equation_total_size(*jt));
	      Equation_vertical_adjust(w.dy,w.y,h,y);
	      col_widths[j]=max(col_widths[j],w.dx);
	    }
	    if (i)
	      row_heights[i]=row_heights[i-1]+h+a.fontsize/2;
	    else
	      row_heights[i]=h;
	    row_bases[i]=y;
	    all_sizes.push_back(tmp);
	  }
	  // accumulate col widths
	  col_widths.front() +=(3*a.fontsize)/2;
	  vector<int>::iterator iit=col_widths.begin()+1,iitend=col_widths.end();
	  for (;iit!=iitend;++iit)
	    *iit += *(iit-1)+a.fontsize;
	  // translate each cell
	  it=all_sizes.begin();
	  itend=all_sizes.end();
	  int h,y,prev_h=0;
	  for (int i=0;it!=itend;++it,++i){
	    h=row_heights[i];
	    y=row_bases[i];
	    iterateur jt=it->_VECTptr->begin(),jtend=it->_VECTptr->end();
	    for (int j=0;jt!=jtend;++jt,++j){
	      eqwdata w(Equation_total_size(*jt));
	      if (j)
		Equation_translate(*jt,col_widths[j-1]-w.x,-h-y);
	      else
		Equation_translate(*jt,-w.x+a.fontsize/2,-h-y);
	    }
	    it->_VECTptr->push_back(eqwdata(col_widths.back(),h-prev_h,0,-h,a,mkvect,0));
	    prev_h=h;
	  }
	  all_sizes.push_back(eqwdata(col_widths.back(),row_heights.back(),0,-row_heights.back(),a,mkmat,-row_heights.back()/2));
	  gen all_sizesg=all_sizes; Equation_translate(all_sizesg,0,row_heights.back()/2); return all_sizesg;
	}
      } // end matrices
      /*************************
       *   SEQUENCES/VECTORS   *
       *************************/
      // horizontal gluing
      if (g.subtype!=_PRINT__VECT) x += a.fontsize/2;
      int ncols=itend-it;
      //ncols=min(ncols,5);
      for (;it!=itend;++it){
	gen cur_size=Equation_compute_size(*it,a,
					   max(windowhsize/ncols-a.fontsize,
#ifdef IPAQ
					       200
#else
					       480
#endif
					       ),contextptr);
	eqwdata tmp=Equation_total_size(cur_size);
	Equation_translate(cur_size,x-tmp.x,0); v.push_back(cur_size);
	x=x+tmp.dx+((g.subtype==_PRINT__VECT)?2:a.fontsize);
	Equation_vertical_adjust(tmp.dy,tmp.y,h,y);
      }
      gen mkvect(at_makevector);
      if (g.subtype==_SEQ__VECT)
	mkvect=at_makesuite;
      else
	mkvect.subtype=g.subtype;
      v.push_back(eqwdata(x,h,0,y,a,mkvect,0));
      return gen(v,_EQW__VECT);
    } // end sequences
    if (g.type==_MOD){ 
      int x=0;
      int h=a.fontsize;
      int y=0;
      bool py=python_compat(contextptr);
      int modsize=int(text_width(a.fontsize,(py?" mod":"%")))+4;
      bool paren=is_positive(-*g._MODptr,contextptr);
      int llp=int(text_width(a.fontsize,("(")));
      int lrp=int(text_width(a.fontsize,(")")));
      gen varg1=Equation_compute_size(*g._MODptr,a,windowhsize,contextptr);
      if (paren) Equation_translate(varg1,llp,0);
      eqwdata vv=Equation_total_size(varg1);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      gen arg2=*(g._MODptr+1);
      gen varg2=Equation_compute_size(arg2,a,windowhsize,contextptr);
      if (paren)
	Equation_translate(varg2,vv.dx+modsize+lrp,0);
      else
	Equation_translate(varg2,vv.dx+modsize,0);
      vv=Equation_total_size(varg2);
      Equation_vertical_adjust(vv.dy,vv.y,h,y);
      vecteur res(makevecteur(varg1,varg2));
      res.push_back(eqwdata(vv.dx+vv.x,h,0,y,a,at_normalmod,0));
      return gen(res,_SEQ__VECT);
    }
    if (g.type!=_SYMB){
      string s=g.type==_STRNG?*g._STRNGptr:g.print(contextptr);
      //if (g==cst_pi) s=char(129);
      if (s.size()>2000)
	s=s.substr(0,2000)+"...";
      int i=int(text_width(a.fontsize,(s.c_str())));
      gen tmp=eqwdata(i,a.fontsize,0,0,a,g);
      return tmp;
    }
    /**********************
     *  SYMBOLIC HANDLING *
     **********************/
    return Equation_compute_symb_size(g,a,windowhsize,contextptr);
    // return Equation_compute_symb_size(aplatir_fois_plus(g),a,windowhsize,contextptr);
    // aplatir_fois_plus is a problem for Equation_replace_selection
    // because it will modify the structure of the data
  }

  void Equation_draw(const eqwdata & e,int x,int y,int rightx,int lowery,Equation * eq){
    if ( (e.dx+e.x<x) || (e.x>rightx) || (e.y>y) || e.y+e.dy<lowery)
      ; // return; // nothing to draw, out of window
    gen gg=e.g;
    int fontsize=e.eqw_attributs.fontsize;
    int text_color=COLOR_BLACK;
    int background=COLOR_WHITE;
    string s=gg.type==_STRNG?*gg._STRNGptr:gg.print(contextptr);
    if (gg.type==_IDNT && !s.empty() && s[0]=='_')
      s=s.substr(1,s.size()-1);
    // if (gg==cst_pi){      s="p";      s[0]=(unsigned char)129;    }
    if (s.size()>2000)
      s=s.substr(0,2000)+"...";
    // cerr << s.size() << endl;
    text_print(fontsize,s.c_str(),eq->x()+e.x-x,eq->y()+y-e.y,text_color,background,e.selected?4:0);
    return;
  }

  inline void check_fl_rectf(int x,int y,int w,int h,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    drawRectangle(x+delta_i,y+delta_j,w,h,c);
    //fl_rectf(x+delta_i,y+delta_j,w,h,c);
  }

  void Equation_draw(const gen & g,int x,int y,int rightx,int lowery,Equation * equat){
    int eqx=equat->x(),eqy=equat->y();
    if (g.type==_EQW){ // terminal
      eqwdata & e=*g._EQWptr;
      Equation_draw(e,x,y,rightx,lowery,equat);
    }
    if (g.type!=_VECT)
      return;
    vecteur & v=*g._VECTptr;
    if (v.empty())
      return;
    gen tmp=v.back();
    if (tmp.type!=_EQW){
      cout << "EQW error:" << v << endl;
      return;
    }
    eqwdata & w=*tmp._EQWptr;
    if ( (w.dx+w.x-x<0) || (w.x>rightx) || (w.y>y) || (w.y+w.dy<lowery) )
      ; // return; // nothing to draw, out of window
    /*******************
     * draw the vector *
     *******************/
    // v is the vector, w the master operator eqwdata
    gen oper=w.g; 
    bool selected=w.selected ;
    int fontsize=w.eqw_attributs.fontsize;
    int background=w.eqw_attributs.background;
    int text_color=w.eqw_attributs.text_color;
    int mode=selected?4:0;
    int draw_line_color=selected?background:text_color;
    int x0=w.x;
    int y0=w.y; // lower coordinate of the master vector
    int y1=y0+w.dy; // upper coordinate of the master vector
    if (selected)
      drawRectangle(eqx+w.x-x,eqy+y-w.y-w.dy+1,w.dx,w.dy+1,text_color);
    // draw arguments of v
    const_iterateur it=v.begin(),itend=v.end()-1;
    if (oper==at_expr && v.size()==3){
      Equation_draw(*it,x,y,rightx,lowery,equat);
      return;
    }
    for (;it!=itend;++it)
      Equation_draw(*it,x,y,rightx,lowery,equat);
    if (oper==at_multistring)
      return;
    string s;
    if (oper.type==_FUNC){
      // catch here special cases user function, vect/matr, ^, int, sqrt, etc.
      unary_function_ptr & u=*oper._FUNCptr;
      if (u==at_at){ // draw brackets around 2nd arg
	gen arg2=v[1]; // 2nd arg of at_of, i.e. what's inside the parenth.
	eqwdata varg2=Equation_total_size(arg2);
	x0=varg2.x;
	y0=varg2.y;
	y1=y0+varg2.dy;
	fontsize=varg2.eqw_attributs.fontsize;
	if (x0<rightx)
	  text_print(fontsize,"[",eqx+x0-x-int(text_width(fontsize,("["))),eqy+y-varg2.baseline,text_color,background,mode);
	x0 += varg2.dx ;
	if (x0<rightx)
	  text_print(fontsize,"]",eqx+x0-x,eqy+y-varg2.baseline,text_color,background,mode);
	return;
      }
      if (u==at_of){ // do we need to draw some parenthesis?
	gen arg2=v[1]; // 2nd arg of at_of, i.e. what's inside the parenth.
	if (arg2.type!=_VECT || arg2._VECTptr->back().type !=_EQW || arg2._VECTptr->back()._EQWptr->g!=at_makesuite){ // Yes (if not _EQW it's a sequence with parent)
	  eqwdata varg2=Equation_total_size(arg2);
	  x0=varg2.x;
	  y0=varg2.y;
	  y1=y0+varg2.dy;
	  fontsize=varg2.eqw_attributs.fontsize;
	  int pfontsize=max(fontsize,(fontsize+(varg2.baseline-varg2.y))/2);
	  if (x0<rightx)
	    text_print(pfontsize,"(",eqx+x0-x-int(text_width(fontsize,("("))),eqy+y-varg2.baseline,text_color,background,mode);
	  x0 += varg2.dx ;
	  if (x0<rightx)
	    text_print(pfontsize,")",eqx+x0-x,eqy+y-varg2.baseline,text_color,background,mode);
	}
	return;
      }
      if (u==at_makesuite){
	bool paren=v.size()!=2; // Sequences with 1 arg don't show parenthesis
	int pfontsize=max(fontsize,(fontsize+(w.baseline-w.y))/2);
	if (paren && x0<rightx)
	  text_print(pfontsize,"(",eqx+x0-x-int(text_width(fontsize,("(")))/2,eqy+y-w.baseline,text_color,background,mode);
	x0 += w.dx;
	if (paren && x0<rightx)
	  text_print(pfontsize,")",eqx+x0-x-int(text_width(fontsize,("(")))/2,eqy+y-w.baseline,text_color,background,mode);
	// print commas between args
	it=v.begin(),itend=v.end()-2;
	for (;it!=itend;++it){
	  eqwdata varg2=Equation_total_size(*it);
	  fontsize=varg2.eqw_attributs.fontsize;
	  if (varg2.x+varg2.dx<rightx)
	    text_print(fontsize,",",eqx+varg2.x+varg2.dx-x+1,eqy+y-varg2.baseline,text_color,background,mode);
	}
	return;
      }
      if (u==at_makevector){ // draw [] delimiters for vector/matrices
	if (oper.subtype!=_SEQ__VECT && oper.subtype!=_PRINT__VECT){
	  int decal=1;
	  switch (oper.subtype){
	  case _MATRIX__VECT: decal=2; break;
	  case _SET__VECT: decal=4; break;
	  case _POLY1__VECT: decal=6; break;
	  }
	  if (eqx+x0-x+1>=0){
	    draw_line(eqx+x0-x+1,eqy+y-y0+1,eqx+x0-x+1,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x+decal,eqy+y-y0+1,eqx+x0-x+decal,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x+1,eqy+y-y0+1,eqx+x0-x+fontsize/4,eqy+y-y0+1,draw_line_color);
	    draw_line(eqx+x0-x+1,eqy+y-y1+1,eqx+x0-x+fontsize/4,eqy+y-y1+1,draw_line_color);
	  }
	  x0 += w.dx ;
	  if (eqx+x0-x-1<LCD_WIDTH_PX){
	    draw_line(eqx+x0-x-1,eqy+y-y0+1,eqx+x0-x-1,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x-decal,eqy+y-y0+1,eqx+x0-x-decal,eqy+y-y1+1,draw_line_color);
	    draw_line(eqx+x0-x-1,eqy+y-y0+1,eqx+x0-x-fontsize/4,eqy+y-y0+1,draw_line_color);
	    draw_line(eqx+x0-x-1,eqy+y-y1+1,eqx+x0-x-fontsize/4,eqy+y-y1+1,draw_line_color);
	  }
	} // end if oper.subtype!=SEQ__VECT
	if (oper.subtype!=_MATRIX__VECT && oper.subtype!=_PRINT__VECT){
	  // print commas between args
	  it=v.begin(),itend=v.end()-2;
	  for (;it!=itend;++it){
	    eqwdata varg2=Equation_total_size(*it);
	    fontsize=varg2.eqw_attributs.fontsize;
	    if (varg2.x+varg2.dx<rightx)
	      text_print(fontsize,",",eqx+varg2.x+varg2.dx-x+1,eqy+y-varg2.baseline,text_color,background,mode);
	  }
	}
	return;
      }
      int lpsize=int(text_width(fontsize,("(")));
      int rpsize=int(text_width(fontsize,(")")));
      eqwdata tmp=Equation_total_size(v.front()); // tmp= 1st arg eqwdata
      if (u==at_sto)
	tmp=Equation_total_size(v[1]);
      x0=w.x-x;
      y0=y-w.baseline;
      if (u==at_pow){
	if (!need_parenthesis(tmp.g)&& tmp.g!=at_pow && tmp.g!=at_prod && tmp.g!=at_division)
	  return;
	if (tmp.g==at_pow){
	  fontsize=tmp.eqw_attributs.fontsize+2;
	}
	if (tmp.x-lpsize<rightx)
	  text_print(fontsize,"(",eqx+tmp.x-x-lpsize,eqy+y-tmp.baseline,text_color,background,mode);
	if (tmp.x+tmp.dx<rightx)
	  text_print(fontsize,")",eqx+tmp.x+tmp.dx-x,eqy+y-tmp.baseline,text_color,background,mode);
	return;
      }
      if (u==at_program){
	if (tmp.x+tmp.dx<rightx)
	  text_print(fontsize,"->",eqx+tmp.x+tmp.dx-x,eqy+y-tmp.baseline,text_color,background,mode);
	return;
      }
#if 1
      if (u==at_sum){
	if (x0<rightx){
	  draw_line(eqx+x0,eqy+y0,eqx+x0+(2*fontsize)/3,eqy+y0,draw_line_color);
	  draw_line(eqx+x0,eqy+y0-fontsize,eqx+x0+(2*fontsize)/3,eqy+y0-fontsize,draw_line_color);
	  draw_line(eqx+x0,eqy+y0,eqx+x0+fontsize/2,eqy+y0-fontsize/2,draw_line_color);
	  draw_line(eqx+x0+fontsize/2,eqy+y0-fontsize/2,eqx+x0,eqy+y0-fontsize,draw_line_color);
	  if (v.size()>2){ // draw the =
	    eqwdata ptmp=Equation_total_size(v[1]);
	    if (ptmp.x+ptmp.dx<rightx)
	      text_print(fontsize,"=",eqx+ptmp.x+ptmp.dx-x-2,eqy+y-ptmp.baseline,text_color,background,mode);
	  }
	}
	return;
      }
#endif
      if (u==at_abs){
	y0 =1+y-w.y;
	int h=w.dy;
	if (x0<rightx){
	  draw_line(eqx+x0+2,eqy+y0-1,eqx+x0+2,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+1,eqy+y0-1,eqx+x0+1,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+w.dx-1,eqy+y0-1,eqx+x0+w.dx-1,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+w.dx,eqy+y0-1,eqx+x0+w.dx,eqy+y0-h+3,draw_line_color);
	}
	return;
      }
      if (u==at_sqrt){
	y0 =1+y-w.y;
	int h=w.dy;
	if (x0<rightx){
	  draw_line(eqx+x0+2,eqy+y0-h/2,eqx+x0+fontsize/2,eqy+y0-1,draw_line_color);
	  draw_line(eqx+x0+fontsize/2,eqy+y0-1,eqx+x0+fontsize,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+fontsize,eqy+y0-h+3,eqx+x0+w.dx-1,eqy+y0-h+3,draw_line_color);
	  ++y0;
	  draw_line(eqx+x0+2,eqy+y0-h/2,eqx+x0+fontsize/2,eqy+y0-1,draw_line_color);
	  draw_line(eqx+x0+fontsize/2,eqy+y0-1,eqx+x0+fontsize,eqy+y0-h+3,draw_line_color);
	  draw_line(eqx+x0+fontsize,eqy+y0-h+3,eqx+x0+w.dx-1,eqy+y0-h+3,draw_line_color);
	}
	return;
      }
      if (u==at_factorial){
	text_print(fontsize,"!",eqx+w.x+w.dx-4-x,eqy+y-w.baseline,text_color,background,mode);
	if (!need_parenthesis(tmp.g)
	    && tmp.g!=at_pow && tmp.g!=at_prod && tmp.g!=at_division
	    )
	  return;
	if (tmp.x-lpsize<rightx)
	  text_print(fontsize,"(",eqx+tmp.x-x-lpsize,eqy+y-tmp.baseline,text_color,background,mode);
	if (tmp.x+tmp.dx<rightx)
	  text_print(fontsize,")",eqx+tmp.x+tmp.dx-x,eqy+y-tmp.baseline,text_color,background,mode);
	return;
      }
#if 1
      if (u==at_integrate){
	x0+=2;
	y0+=fontsize/2;
	if (x0<rightx){
	  fl_arc(eqx+x0,eqy+y0,fontsize/3,fontsize/3,180,360,draw_line_color);
	  draw_line(eqx+x0+fontsize/3,eqy+y0,eqx+x0+fontsize/3,eqy+y0-2*fontsize+4,draw_line_color);
	  fl_arc(eqx+x0+fontsize/3,eqy+y0-2*fontsize+3,fontsize/3,fontsize/3,0,180,draw_line_color);
	}
	if (v.size()!=2){ // if arg has size > 1 draw the d
	  eqwdata ptmp=Equation_total_size(v[1]);
	  if (ptmp.x<rightx)
	    text_print(fontsize," d",eqx+ptmp.x-x-int(text_width(fontsize,(" d"))),eqy+y-ptmp.baseline,text_color,background,mode);
	}
	else {
	  eqwdata ptmp=Equation_total_size(v[0]);
	  if (ptmp.x+ptmp.dx<rightx)
	    text_print(fontsize," dx",eqx+ptmp.x+ptmp.dx-x,eqy+y-ptmp.baseline,text_color,background,mode);
	}
	return;
      }
#endif
      if (u==at_division){
	if (x0<rightx){
	  int yy=eqy+y0-8;
	  draw_line(eqx+x0+2,yy,eqx+x0+w.dx-2,yy,draw_line_color);
	  ++yy;
	  draw_line(eqx+x0+2,yy,eqx+x0+w.dx-2,yy,draw_line_color);
	}
	return;
      }
#if 1
      if (u==at_limit && v.size()>=4){
	if (x0<rightx)
	  text_print(fontsize,"lim",eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
	gen arg2=v[1]; // 2nd arg of limit, i.e. the variable
	if (arg2.type==_EQW){ 
	  eqwdata & varg2=*arg2._EQWptr;
	  if (varg2.x+varg2.dx+2<rightx)
	    text_print(fontsize,"\x1e",eqx+varg2.x+varg2.dx+2-x,eqy+y-varg2.y,text_color,background,mode);
	}
	if (v.size()>=5){
	  arg2=v[2]; // 3rd arg of lim, the point, draw a comma after if dir.
	  if (arg2.type==_EQW){ 
	    eqwdata & varg2=*arg2._EQWptr;
	    if (varg2.x+varg2.dx<rightx)
	      text_print(fontsize,",",eqx+varg2.x+varg2.dx-x,eqy+y-varg2.baseline,text_color,background,mode);
	  }
	}
	return;
      } // limit
#endif
      bool parenthesis=true;
      string opstring(",");
      if (u.ptr()->printsommet==&printsommetasoperator || binary_op(u) ){
	if (u==at_normalmod && python_compat(contextptr))
	  opstring=" mod";
	else
	  opstring=u.ptr()->s;
      }
      else {
	if (u==at_sto)
	  opstring=":=";
	parenthesis=false;
      }
      // int yy=y0; // y0 is the lower coordinate of the whole eqwdata
      // int opsize=int(text_width(fontsize,(opstring.c_str())))+3;
      it=v.begin();
      itend=v.end()-1;
      // Reminder: here tmp is the 1st arg eqwdata, w the whole eqwdata
      if ( (itend-it==1) && ( (u==at_neg) 
			      || (u==at_plus) // uncommented for +infinity
			      ) ){ 
	if ( (u==at_neg &&need_parenthesis(tmp.g) && tmp.g!=at_prod)){
	  if (tmp.x-lpsize<rightx)
	    text_print(fontsize,"(",eqx+tmp.x-x-lpsize,eqy+y-tmp.baseline,text_color,background,mode);
	  if (tmp.x+tmp.dx<rightx)
	    text_print(fontsize,")",eqx+tmp.x-x+tmp.dx,eqy+y-tmp.baseline,text_color,background,mode);
	}
	if (w.x<rightx){
	  text_print(fontsize,u.ptr()->s,eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
	}
	return;
      }
      // write first open parenthesis
      if (u==at_plus && tmp.g!=at_equal)
	parenthesis=false;
      else {
	if (parenthesis && need_parenthesis(tmp.g)){
	  if (w.x<rightx){
	    int pfontsize=max(fontsize,(fontsize+(tmp.baseline-tmp.y))/2);
	    text_print(pfontsize,"(",eqx+w.x-x,eqy+y-tmp.baseline,text_color,background,mode);
	  }
	}
      }
      for (;;){
	// write close parenthesis at end
	int xx=tmp.dx+tmp.x-x;
	if (parenthesis && need_parenthesis(tmp.g)){
	  if (xx<rightx){
	    int pfontsize=min(max(fontsize,(fontsize+(tmp.baseline-tmp.y))/2),fontsize*2);
	    int deltapary=(2*(pfontsize-fontsize))/3;
	    text_print(pfontsize,")",eqx+xx,eqy+y-tmp.baseline+deltapary,text_color,background,mode);
	  }
	  xx +=rpsize;
	}
	++it;
	if (it==itend){
	  if (u.ptr()->printsommet==&printsommetasoperator || u==at_sto || binary_op(u))
	    return;
	  else
	    break;
	}
	// write operator
	if (u==at_prod){
	  // text_print(fontsize,".",eqx+xx+3,eqy+y-tmp.baseline-fontsize/3);
	  text_print(fontsize,opstring.c_str(),eqx+xx+1,eqy+y-tmp.baseline,text_color,background,mode);
	}
	else {
	  gen tmpgen;
	  if (u==at_plus && ( 
			     (it->type==_VECT && it->_VECTptr->back().type==_EQW && it->_VECTptr->back()._EQWptr->g==at_neg) 
			     || 
			     ( it->type==_EQW && (is_integer(it->_EQWptr->g) || it->_EQWptr->g.type==_DOUBLE_) && is_strictly_positive(-it->_EQWptr->g,contextptr) ) 
			      )
	      )
	    ;
	  else {
	    if (xx+1<rightx)
	      // fl_draw(opstring.c_str(),xx+1,y-tmp.y-tmp.dy/2+fontsize/2);
	      text_print(fontsize,opstring.c_str(),eqx+xx+1,eqy+y-tmp.baseline,text_color,background,mode);
	  }
	}
	// write right parent, update tmp
	tmp=Equation_total_size(*it);
	if (parenthesis && (need_parenthesis(tmp.g)) ){
	  if (tmp.x-lpsize<rightx){
	    int pfontsize=min(max(fontsize,(fontsize+(tmp.baseline-tmp.y))/2),fontsize*2);
	    int deltapary=(2*(pfontsize-fontsize))/3;
	    text_print(pfontsize,"(",eqx+tmp.x-pfontsize*lpsize/fontsize-x,eqy+y-tmp.baseline+deltapary,text_color,background,mode);
	  }
	}
      } // end for (;;)
      if (w.x<rightx){
	s = u.ptr()->s;
	s += '(';
	text_print(fontsize,s.c_str(),eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
      }
      if (w.x+w.dx-rpsize<rightx)
	text_print(fontsize,")",eqx+w.x+w.dx-x-rpsize+2,eqy+y-w.baseline,text_color,background,mode);
      return;
    }
    s=oper.print(contextptr);
    if (w.x<rightx){
      text_print(fontsize,s.c_str(),eqx+w.x-x,eqy+y-w.baseline,text_color,background,mode);
    }
  }

  Equation::Equation(int x_, int y_, const gen & g){
    _x=x_;
    _y=y_;
    attr=attributs(18,COLOR_WHITE,COLOR_BLACK);
    if (taille(g,max_prettyprint_equation)<max_prettyprint_equation)
      data=Equation_compute_size(g,attr,LCD_WIDTH_PX,contextptr);
    else
      data=Equation_compute_size(string2gen("Object_too_large",false),attr,LCD_WIDTH_PX,contextptr);
    undodata=Equation_copy(data);
  }

  void replace_selection(Equation & eq,const gen & tmp,gen * gsel,const vector<int> * gotoptr){
    int xleft,ytop,xright,ybottom,gselpos; gen *gselparent;
    vector<int> goto_sel;
    eq.undodata=Equation_copy(eq.data);
    if (gotoptr==0){
      if (xcas::Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos,&goto_sel) && gsel)
	gotoptr=&goto_sel;
      else
	return;
    }
    *gsel=xcas::Equation_compute_size(tmp,eq.attr,LCD_WIDTH_PX,contextptr);
    gen value;
    xcas::do_select(eq.data,true,value);
    if (value.type==_EQW)
      eq.data=xcas::Equation_compute_size(value._EQWptr->g,eq.attr,LCD_WIDTH_PX,contextptr);
    //cout << "new value " << value << " " << eq.data << " " << *gotoptr << endl;
    xcas::Equation_select(eq.data,false);
    gen * gptr=&eq.data;
    for (int i=gotoptr->size()-1;i>=0;--i){
      int pos=(*gotoptr)[i];
      if (gptr->type==_VECT &&gptr->_VECTptr->size()>pos)
	gptr=&(*gptr->_VECTptr)[pos];
    }
    xcas::Equation_select(*gptr,true);
    //cout << "new sel " << *gptr << endl;
  }

  void display(Equation & eq,int x,int y){
    // Equation_draw(eq.data,x,y,LCD_WIDTH_PX,0,&eq);
    int xleft,ytop,xright,ybottom,gselpos; gen * gsel,*gselparent;
    eqwdata eqdata=Equation_total_size(eq.data);
    if ( (eqdata.dx>LCD_WIDTH_PX || eqdata.dy>LCD_HEIGHT_PX-STATUS_AREA_PX) && Equation_adjust_xy(eq.data,xleft,ytop,xright,ybottom,gsel,gselparent,gselpos)){
      if (x<xleft){
	if (x+LCD_WIDTH_PX<xright)
	  x=giacmin(xleft,xright-LCD_WIDTH_PX);
      }
      if (x>=xleft && x+LCD_WIDTH_PX>=xright){
	if (xright-x<LCD_WIDTH_PX)
	  x=giacmax(xright-LCD_WIDTH_PX,0);
      }
#if 0
      cout << "avant " << y << " " << ytop << " " << ybottom << endl;
      if (y<ytop){
	if (y+LCD_HEIGHT_PX<ybottom)
	  y=giacmin(ytop,ybottom-LCD_HEIGHT_PX);
      }
      if (y>=ytop && y+LCD_HEIGHT_PX>=ybottom){
	if (ybottom-y<LCD_HEIGHT_PX)
	  y=giacmax(ybottom-LCD_HEIGHT_PX,0);
      }
      cout << "apres " << y << " " << ytop << " " << ybottom << endl;
#endif
    }
    int save_ymin_clip=clip_ymin;
    clip_ymin=STATUS_AREA_PX;
    Equation_draw(eq.data,x,y,RAND_MAX,0,&eq);
    clip_ymin=save_ymin_clip;
  }
  
  /* ******************* *
   *      GRAPH          *
   * ******************* *
   */
#if 1

  double find_tick(double dx){
    double d=std::pow(10.0,std::floor(std::log10(absdouble(dx))));
    if (dx<2*d)
      d=d/5;
    else {
      if (dx<5*d)
	d=d/2;
    }
    return d;
  }

  // check if point is inside triangle
  inline bool inside(double x0,double x1,double x2,
		     double y0,double y1,double y2,
		     double x,double y){
#if 1
    double y02=y2-y0,y10=y0-y1;
    double invarea = x0*(y1-y2) + x1*y02 + x2*y10;
    if (invarea==0) return false;
    invarea=1/invarea;
    double yy0=y0-y, s=invarea*(x*y02 + x2*yy0 + x0*(y-y2));
    if (s<-1e-12) return false;
    double t = invarea*(x*y10 + x0*(y1-y) - x1*yy0);
    bool res=t>=-1e-12 && s+t<=1+1e-12;
    // if (res) cout << "inside "<< "\n";
    return res;
#else
    double as_x = x-x0;
    double as_y = y-y0;
    bool s_01 = (x1-x0)*as_y-(y1-y0)*as_x>0; // dot product P0P1.P0P>0
    if ( (x2-x0)*as_y-(y2-y0)*as_x>0 == s_01)
      return false;
    if ((x2-x1)*(y-y1)-(y2-y1)*(x-x1) > 0 != s_01)
      return false;
    return true;
#endif
  }

  /* 3d rotation handling */
  void normalize(double & a,double &b,double &c){
    double n=std::sqrt(a*a+b*b+c*c);
    a /= n;
    b /= n;
    c /= n;
  }

  inline int Min(int i,int j) {return i>j?j:i;}

  inline int Max(int i,int j) {return i>j?i:j;}

  quaternion_double::quaternion_double(double theta_x,double theta_y,double theta_z) { 
    *this=euler_deg_to_quaternion_double(theta_x,theta_y,theta_z); 
  }

  quaternion_double euler_deg_to_quaternion_double(double a,double b,double c){
    double phi=a*M_PI/180, theta=b*M_PI/180, psi=c*M_PI/180;
    double c1 = std::cos(phi/2);
    double s1 = std::sin(phi/2);
    double c2 = std::cos(theta/2);
    double s2 = std::sin(theta/2);
    double c3 = std::cos(psi/2);
    double s3 = std::sin(psi/2);
    double c1c2 = c1*c2;
    double s1s2 = s1*s2;
    double w =c1c2*c3 - s1s2*s3;
    double x =c1c2*s3 + s1s2*c3;
    double y =s1*c2*c3 + c1*s2*s3;
    double z =c1*s2*c3 - s1*c2*s3;
    return quaternion_double(w,x,y,z);
  }

  void quaternion_double_to_euler_deg(const quaternion_double & q,double & phi,double & theta, double & psi){
    double test = q.x*q.y + q.z*q.w;
    if (test > 0.499) { // singularity at north pole
      phi = 2 * std::atan2(q.x,q.w) * 180/M_PI;
      theta = 90; 
      psi = 0;
      return;
    }
    if (test < -0.499) { // singularity at south pole
      phi = -2 * std::atan2(q.x,q.w) * 180/M_PI;
      theta = - 90;
      psi = 0;
      return;
    }
    double sqx = q.x*q.x;
    double sqy = q.y*q.y;
    double sqz = q.z*q.z;
    phi = std::atan2(2*q.y*q.w-2*q.x*q.z , 1 - 2*sqy - 2*sqz) * 180/M_PI;
    theta = std::asin(2*test) * 180/M_PI;
    psi = std::atan2(2*q.x*q.w-2*q.y*q.z , 1 - 2*sqx - 2*sqz) * 180/M_PI;
  }

  quaternion_double operator * (const quaternion_double & q1,const quaternion_double & q2){ 
    double z=q1.w*q2.z+q2.w*q1.z+q1.x*q2.y-q2.x*q1.y;
    double x=q1.w*q2.x+q2.w*q1.x+q1.y*q2.z-q2.y*q1.z;
    double y=q1.w*q2.y+q2.w*q1.y+q1.z*q2.x-q2.z*q1.x;
    double w=q1.w*q2.w-q1.x*q2.x-q1.y*q2.y-q1.z*q2.z;
    return quaternion_double(w,x,y,z);
  }

  // q must be a unit
  void get_axis_angle_deg(const quaternion_double & q,double &x,double &y,double & z, double &theta){
    double scale=1-q.w*q.w;
    if (scale>1e-6){
      scale=std::sqrt(scale);
      theta=2*std::acos(q.w)*180/M_PI;
      x=q.x/scale;
      y=q.y/scale;
      z=q.z/scale;
    }
    else {
      x=0; y=0; z=1;
      theta=0;
    }
  }

  quaternion_double rotation_2_quaternion_double(double x, double y, double z,double theta){
    double t=theta*M_PI/180;
    double qx,qy,qz,qw,s=std::sin(t/2),c=std::cos(t/2);
    qx=x*s;
    qy=y*s;
    qz=z*s;
    qw=c;
    double n=std::sqrt(qx*qx+qy*qy+qz*qz+qw*qw);
    return quaternion_double(qw/n,qx/n,qy/n,qz/n);
  }

  // image of (x,y,z) by rotation around axis r(rx,ry,rz) of angle theta
  void rotate(double rx,double ry,double rz,double theta,double x,double y,double z,double & X,double & Y,double & Z){
    /*
    quaternion_double q=rotation_2_quaternion_double(rx,ry,rz,theta);
    quaternion_double qx(x,y,z,0);
    quaternion_double qX=conj(q)*qx*q;
    */
    // r(rx,ry,rz) the axis, v(x,y,z) projects on w=a*r with a such that
    // w.r=a*r.r=v.r
    double r2=rx*rx+ry*ry+rz*rz;
    double r=std::sqrt(r2);
    double a=(rx*x+ry*y+rz*z)/r2;
    // v=w+V, w remains stable, V=v-w=v-a*r rotates
    // Rv=w+RV, where RV=cos(theta)*V+sin(theta)*(r cross V)/sqrt(r2)
    double Vx=x-a*rx,Vy=y-a*ry,Vz=z-a*rz;
    // cross product of k with V
    double kVx=ry*Vz-rz*Vy, kVy=rz*Vx-rx*Vz,kVz=rx*Vy-ry*Vx;
    double c=std::cos(theta),s=std::sin(theta);
    X=a*rx+c*Vx+s*kVx/r;
    Y=a*ry+c*Vy+s*kVy/r;
    Z=a*rz+c*Vz+s*kVz/r;
  }

  int diffuse(int color_orig,float3d diffusionz){
    if (diffusionz<1.1)
      return color_orig;
    int color=rgb565to888(color_orig);
    int r=(color&0xff0000)>>16,g=(color & 0xff00)>>8,b=192;
    float3d attenuate=(.3*(diffusionz-1));
    attenuate=1.0/(1+attenuate*attenuate);
    r*=attenuate; g*=attenuate; b*=attenuate;
    return rgb888to565((r<<16)|(g<<8)|b);
  }
  
  void glinter1(float3d z,float3d dz,
		float3d *zmin,float3d *zmax,float3d ZMIN,float3d ZMAX,
		int ih,int lcdz,
		int upcolor,int downcolor,int diffusionz,int diffusionz_limit,bool interval
		){
    if (ZMIN<z && z<ZMAX)
      return;
    // lcdz tests below: avoid marking too large regions
    if (*zmax<*zmin || z<*zmin-lcdz || z>*zmax+lcdz)
      *zmax=*zmin=z;
    bool diffus=diffusionz<diffusionz_limit;
    float3d deltaz;
    if (interval){
      bool intervalonly=false;
      if (z<0) {
	// return;
	z=0; intervalonly=true;
      }
      if (z>=LCD_HEIGHT_PX) {
	// return;
	z=LCD_HEIGHT_PX-1; intervalonly=true;
      }
      deltaz=diffus?1:diffusionz;
      if (z>*zmax+deltaz){
	if (diffus){
	  drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),diffuse(downcolor,diffusionz));
	  if (!intervalonly)
	    os_set_pixel(ih,z,downcolor);
	}
	else {
	  drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),_BLACK);
	  // draw interval
	  int nstep=int(z-*zmax)/diffusionz;
	  float3d zstep=(z-*zmax)/nstep;
	  for (float3d zz=*zmax+zstep;zz<=z;zz+=zstep)
	    os_set_pixel(ih,zz,downcolor);
	}
	*zmax=z;
	return;
      }
      else if (z<*zmin-deltaz){
	if (diffus){
	  drawRectangle(ih,z,1,std::ceil(*zmin-z),diffuse(upcolor,diffusionz));
	  if (!intervalonly)
	    os_set_pixel(ih,z,upcolor);
	}
	else {
	  drawRectangle(ih,z,1,std::ceil(*zmin-z),_BLACK);
	  // draw interval
	  int nstep=int(*zmin-z)/diffusionz;
	  float3d zstep=(z-*zmin)/nstep; // zstep<0
	  for (float3d zz=*zmin+zstep;zz>=z;zz+=zstep)
	    os_set_pixel(ih,zz,upcolor);
	}
	*zmin=z;
	return;
      }
    } // end if interval
    if (z>=0 &&  z<=LCD_HEIGHT_PX){
      int color=-1;
      if (diffus){
	if (z<=*zmin){
	  // mark all points with diffuse color from upcolor
	  drawRectangle(ih,z,1,std::ceil(*zmin-z),diffuse(upcolor,std::min(float3d(diffusionz),std::max(-dz,1.0))));
	  color=upcolor;
	  *zmin=z;
	}
	if (z>=*zmax){
	  // mark all points with diffuse color from downcolor
	  drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),diffuse(downcolor,std::min(float3d(diffusionz),std::max(dz,1.0))));
	  *zmax=z;
	}
	return;
      }
      if (z>*zmax){ // mark only 1 point
	color=downcolor;
	//drawRectangle(ih,*zmax+1,1,z-*zmax-1,_BLACK);
	*zmax=z;
      }
      if (z<*zmin){ // mark 1 point
	color=upcolor;
	drawRectangle(ih,z+1,1,*zmin-z-1,_BLACK);
	*zmin=z;
      }
      if (color>=0) os_set_pixel(ih,z,color);
    }
  }
  
  
  void glinter(float3d a,float3d b,float3d c,
	       float3d xscale,float3d xc,float3d yscale,float3d yc,
	       float3d *zmin,float3d *zmax,float3d ZMIN,float3d ZMAX,
	       int i,int horiz,int j,int w,int h,int lcdz,
	       int upcolor,int downcolor,int diffusionz,int diffusionz_limit,bool interval
	       ){
    float3d dz=lcdz*(a+b)*yscale-1;
    //if (dz<-10 || dz>10) cout << "dz=" << dz << "\n";
    // plane equation solved
    if (//0
	h==1 && w==1
	){
      int ih=i+horiz;
      float3d x = yscale*j-xscale*i + xc;
      // if (x<xmin) continue;
      float3d y = yscale*j+xscale*i + yc;
      // if (y<ymin) continue;
      float3d z = (a*x+b*y+c);
      z=LCD_HEIGHT_PX/2+j-lcdz*z;
      if (ZMIN<z && z<ZMAX)
	return;
      bool intervalonly=false;
      // lcdz tests below: avoid marking too large regions
      if (*zmax<*zmin || z<*zmin-lcdz || z>*zmax+lcdz)
	*zmax=*zmin=z;
      bool diffus=diffusionz<diffusionz_limit;
      float3d deltaz;
      if (interval){
	if (z<0) {
	  // return;
	  z=0; intervalonly=true;
	}
	if (z>=LCD_HEIGHT_PX) {
	  // return;
	  z=LCD_HEIGHT_PX-1; intervalonly=true;
	}
	deltaz=diffus?1:diffusionz;
	if (z>*zmax+deltaz){
	  if (diffus){
	    drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),diffuse(downcolor,diffusionz));
	    if (!intervalonly)
	      os_set_pixel(ih,z,downcolor);
	  }
	  else {
	    drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),_BLACK);
	    // draw interval
	    int nstep=int(z-*zmax)/diffusionz;
	    float3d zstep=(z-*zmax)/nstep;
	    for (float3d zz=*zmax+zstep;zz<=z;zz+=zstep)
	      os_set_pixel(ih,zz,downcolor);
	  }
	  *zmax=z;
	  return;
	}
	else if (z<*zmin-deltaz){
	  if (diffus){
	    drawRectangle(ih,z,1,std::ceil(*zmin-z),diffuse(upcolor,diffusionz));
	    if (!intervalonly)
	      os_set_pixel(ih,z,upcolor);
	  }
	  else {
	    drawRectangle(ih,z,1,std::ceil(*zmin-z),_BLACK);
	    // draw interval
	    int nstep=int(*zmin-z)/diffusionz;
	    float3d zstep=(z-*zmin)/nstep; // zstep<0
	    for (float3d zz=*zmin+zstep;zz>=z;zz+=zstep)
	      os_set_pixel(ih,zz,upcolor);
	  }
	  *zmin=z;
	  return;
	}
      } // end if interval
      if (z>=0 &&  z<=LCD_HEIGHT_PX){
	int color=-1;
	if (diffus){
	  if (z<=*zmin){
	    // mark all points with diffuse color from upcolor
	    drawRectangle(ih,z,1,std::ceil(*zmin-z),diffuse(upcolor,std::min(float3d(diffusionz),std::max(-dz,1.0))));
	    color=upcolor;
	    *zmin=z;
	  }
	  if (z>=*zmax){
	    // mark all points with diffuse color from downcolor
	    drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),diffuse(downcolor,std::min(float3d(diffusionz),std::max(dz,1.0))));
	    *zmax=z;
	  }
	  return;
	}
	if (z>=*zmax){ // mark only 1 point
	  color=downcolor;
	  // drawRectangle(ih,*zmax,1,z-*zmax,color);
	  // drawRectangle(ih,*zmax,1,z-*zmax,_BLACK);
	  *zmax=z;
	}
	if (z<=*zmin){ // mark 1 point
	  color=upcolor;
	  //drawRectangle(ih,z,1,*zmin-z,color);
	  //drawRectangle(ih,z,1,*zmin-z,_BLACK);
	  *zmin=z;
	}
	if (color>=0) os_set_pixel(ih,z,color);
      }
      return; // end h==1 and w==1
    }
    for (int I=i;I<i+w;++I,++zmax,++zmin){
      int ih=I+horiz;
      float3d x = yscale*j-xscale*I + xc;
      // if (x<xmin) continue;
      float3d y = yscale*j+xscale*I + yc;
      // if (y<ymin) continue;
      float3d z = (a*x+b*y+c);
      bool intervalonly=false;
      z=LCD_HEIGHT_PX/2+j-lcdz*z;
      if (ZMIN<z && z<ZMAX)
	return;
      if (interval){
	if (z<0) {
	  z=0; intervalonly=true;
	}
	if (z>=LCD_HEIGHT_PX) {
	  z=LCD_HEIGHT_PX-1; intervalonly=true;
	}
      }
      if (i==0)
	; // cout << "i=" << i << " j=" << j << ", zmin=" << *zmin << " z=" << z << " zmax=" << *zmax << " dz=" << dz << ", a=" << a << " b=" << b << " c=" << c <<"\n";
      if (*zmax<*zmin || z<*zmin-lcdz || z>*zmax+lcdz)
	*zmax=*zmin=z;
      int deltaz=(diffusionz<diffusionz_limit?1:diffusionz);
      if (interval && z>*zmax+deltaz){
	if (//0
	    diffusionz<diffusionz_limit
	    )	  
	  drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),diffuse(downcolor,diffusionz));
	else {
	  drawRectangle(ih,*zmax,1,std::ceil(z-*zmax),_BLACK);
	  // draw interval
	  int nstep=int(z-*zmax)/diffusionz;
	  float3d zstep=(z-*zmax)/nstep;
	  for (float3d zz=*zmax+zstep;zz<=z;zz+=zstep)
	    os_set_pixel(ih,zz,downcolor);
	}
	if (intervalonly){
	  *zmax=z;
	  continue;
	}
      }
      if (interval && z<*zmin-deltaz){
	if (//0
	    diffusionz<diffusionz_limit
	    )
	  drawRectangle(ih,z,1,std::ceil(*zmin-z),diffuse(upcolor,diffusionz));
	else {
	  drawRectangle(ih,z,1,std::ceil(*zmin-z),_BLACK);
	  // draw interval
	  int nstep=int(*zmin-z)/diffusionz;
	  float3d zstep=(z-*zmin)/nstep; // zstep<0
	  for (float3d zz=*zmin+zstep;zz>=z;zz+=zstep)
	    os_set_pixel(ih,zz,upcolor);
	}
	if (intervalonly){
	  *zmin=z;
	  continue;
	}
      }
      if (//x-(h-1)*yscale>xmin && y-(h-1)*yscale>ymin &&
	  z>=0 && z+(h-1)*dz>=0 && z<=LCD_HEIGHT_PX && z+(h-1)*dz<=LCD_HEIGHT_PX
	  ){
	int color=-1;
	if ( (h>1 || diffusionz<diffusionz_limit) && dz>0 && z>=*zmax){
	  // mark all points with downcolor
	  *zmax=z+(h-1)*dz;
	  color=downcolor;
	  if (diffusionz>=diffusionz_limit && dz>diffusionz){
	    drawRectangle(ih,z,1,std::ceil(*zmax-z),_BLACK);
	    // draw interval
	    int nstep=int(std::ceil((*zmax-z)/diffusionz));
	    float3d zstep=(*zmax-z)/nstep;
	    for (int i=0;i<=nstep;++i)
	      os_set_pixel(ih,z+i*zstep,color);		    
	    continue;
	  }
	}
	if ( (h>1 || diffusionz<diffusionz_limit) && dz<0 && z<=*zmin){
	  // mark all points with upcolor
	  *zmin=z+(h-1)*dz;
	  color=upcolor;
	  if (diffusionz>=diffusionz_limit && dz<-diffusionz){
	    drawRectangle(ih,z,1,std::ceil(z-*zmin),_BLACK);
	    // draw interval
	    int nstep=int(std::ceil((z-*zmin)/diffusionz));
	    float3d zstep=(*zmin-z)/nstep;
	    for (int i=0;i<=nstep;++i)
	      os_set_pixel(ih,z+i*zstep,color);
	    continue;
	  }
	}
	if (color>=0){
	  if (diffusionz<diffusionz_limit){
	    if (dz>0)
	      drawRectangle(ih,z,1,std::ceil(h*dz),diffuse(color,std::min(float3d(diffusionz),std::max(dz,1.0))));
	    else
	      drawRectangle(ih,z-std::ceil(-h*dz),1,std::ceil(-h*dz),diffuse(color,std::min(float3d(diffusionz),std::max(-dz,1.0))));
	    continue;
	  }
	  if (dz>1)
	    drawRectangle(ih,z,1,std::ceil(h*dz),_BLACK);
	  os_set_pixel(ih,z,color);
	  if (h==1) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  if (h==2) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  if (h==3) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  if (h==4) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  if (h==5) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  if (h==6) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  if (h==7) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  if (h==8) continue;
	  z += dz;
	  os_set_pixel(ih,z,color);
	  continue;
	}
	if (dz<=0 && z>=*zmax && z+(h-1)*dz>=*zmin){ // mark only 1 point
	  *zmax=z;
	  color=downcolor;
	}
	if (dz>=0 && z<=*zmin && z+(h-1)*dz<=*zmax){ // mark 1 point
	  *zmin=z;
	  color=upcolor;
	}
	if (color>=0){
	  os_set_pixel(ih,z,color);
	  continue;
	}
      }
      for (int J=0;J<h
	     // && x>=xmin && y>=ymin
	     ;++J,z+=dz,x-=yscale,y-=yscale){
	int color=-1;
	if (z>*zmax){
	  drawRectangle(i,*zmax,1,z-*zmax,_BLACK);
	  *zmax=z;
	  color=downcolor;
	}
	if (z<*zmin){
	  drawRectangle(i,z,1,*zmin-z,_BLACK);
	  *zmin=z;
	  color=upcolor;
	}
	if (z<=-0.5 || z>=LCD_HEIGHT_PX)
	  continue;
	if (color>=0)
	  os_set_pixel(ih,z,color); // drawRectangle(i,z,w,h,color);
      }
    }
  }


  void find_abc(float3d x1,float3d x2,float3d x3,
		float3d y1,float3d y2,float3d y3,
		float3d z1,float3d z2,float3d z3,
		float3d &a,float3d &b,float3d &c){
    // solve([a*x1+b*y1+c=z1,a*x2+b*y2+c=z2,a*x3+b*y3+c=z3],[a,b,c])
    // float3d d=(x1*y2-x1*y3-x2*y1+x2*y3+x3*y1-x3*y2);
    float3d d=(x1*(y2-y3)+x2*(y3-y1)+x3*(y1-y2));
    if (d==0) return;
    d=1/d;
    float3d z12=z2-z1,z23=z3-z2,z31=z1-z3;
    //float3d a=(-y1*z2+y1*z3+y2*z1-y2*z3-y3*z1+y3*z2)/d;
    a=d*(y1*z23+y2*z31+y3*z12);
    // float3d b=(x1*z2-x1*z3-x2*z1+x2*z3+x3*z1-x3*z2)/d;
    b=-d*(x1*z23+x2*z31+x3*z12);
    //float3d c=(x1*y2*z3-x1*y3*z2-x2*y1*z3+x2*y3*z1+x3*y1*z2-x3*y2*z1)/d;
    c=d*(x1*(y2*z3-y3*z2)+x2*(y3*z1-y1*z3)+x3*(y1*z2-y2*z1));
  }

  void glinter(float3d x1,float3d x2,float3d x3,
	       float3d y1,float3d y2,float3d y3,
	       float3d z1,float3d z2,float3d z3,
	       float3d xscale,float3d xc,float3d yscale,float3d yc,
	       float3d *zmin,float3d *zmax,float3d ZMIN,float3d ZMAX,
	       int i,int horiz,int j,int w,int h,int lcdz,
	       int upcolor,int downcolor,int diffusionz,int diffusionz_limit,bool interval
	       ){
    float3d a,b,c;
    find_abc(x1,x2,x3,y1,y2,y3,z1,z2,z3,a,b,c);
    glinter(a,b,c,xscale,xc,yscale,yc,zmin,zmax,ZMIN,ZMAX,i,horiz,j,w,h,lcdz,upcolor,downcolor,diffusionz,diffusionz_limit,interval);
  }
  
  void update12(bool & found,bool &found2,
		float3d x1,float3d x2,float3d x3,float3d y1,float3d y2,float3d y3,float3d z1,float3d z2,float3d z3,
		int upcolor,int downcolor,int downupcolor,int downdowncolor,
		float3d & curx1, float3d &curx2, float3d &curx3, float3d &cury1, float3d &cury2, float3d &cury3, float3d &curz1, float3d &curz2, float3d &curz3, 
		float3d &cur2x1, float3d &cur2x2, float3d &cur2x3, float3d &cur2y1, float3d &cur2y2, float3d &cur2y3, float3d &cur2z1, float3d &cur2z2, float3d &cur2z3,
		int & u,int & d,int & du,int & dd){
    //cout << "update12\n";
    if (found){
      if (z1+z2+z3<curz1+curz2+curz3){
	// no need to update cur, perhaps cur2?
	if (found2 && cur2z1+cur2z2+cur2z3<z1+z2+z3)
	  return;
	found2=true;
	cur2x1=x1; cur2x2=x2; cur2x3=x3;
	cur2y1=y1; cur2y2=y2; cur2y3=y3;
	cur2z1=z1; cur2z2=z2; cur2z3=z3;
	du=downupcolor; dd=downdowncolor;
	return;
      }
      else { // need to update cur, and perhaps cur2
	if (!found2 || curz1+curz2+curz3<cur2z1+cur2z2+cur2z3){
	  found2=true;
	  cur2x1=curx1; cur2x2=curx2; cur2x3=curx3;
	  cur2y1=cury1; cur2y2=cury2; cur2y3=cury3;
	  cur2z1=curz1; cur2z2=curz2; cur2z3=curz3;
	  du=downupcolor; dd=downdowncolor;
	}
	curx1=x1; curx2=x2; curx3=x3;
	cury1=y1; cury2=y2; cury3=y3;
	curz1=z1; curz2=z2; curz3=z3;
	u=upcolor; d=downcolor;
	return;
      }
    }
    found=true;
    curx1=x1; curx2=x2; curx3=x3;
    cury1=y1; cury2=y2; cury3=y3;
    curz1=z1; curz2=z2; curz3=z3;
    u=upcolor; d=downcolor;
  }
	      
  void update12(bool & found,bool &found2,
		float3d a,float3d b,float3d c,float3d z,
		int upcolor,int downcolor,int downupcolor,int downdowncolor,
		float3d & cura1,float3d & curb1,float3d & curc1,float3d & curz1,
		float3d & cura2,float3d & curb2,float3d & curc2,float3d & curz2,
		int & u,int & d,int & du,int & dd){
    if (!found || z>curz1){
      if (found){ // update cur2
	found2=true;
	cura2=cura1; curb2=curb1; curc2=curc1; curz2=curz1;
      }
      found=true;
      cura1=a; curb1=b; curc1=c; curz1=z;
      u=upcolor; d=downcolor;
      return;
    }
    if (z>curz2){
      found2=true;
      cura2=a; curb2=b; curc2=c; curz2=z;
      du=downupcolor; dd=downdowncolor;
    }
  }
	      
  
  // 3d demo prototype
  inline void do_transform(const float3d mat[16],float3d x,float3d y,float3d z,float3d & X,float3d & Y,float3d &Z){
    X=mat[0]*x+mat[1]*y+mat[2]*z+mat[3];
    Y=mat[4]*x+mat[5]*y+mat[6]*z+mat[7];
    Z=mat[8]*x+mat[9]*y+mat[10]*z+mat[11];
    // float3d t=mat[12]*x+mat[13]*y+mat[14]*z+mat[15];
    // X/=t; Y/=t; Z/=t;
  }

#if 1
  bool inside(const vector<float3d3> & v,float3d x,float3d y){
    int n=0;
    for (int i=1;i<v.size();++i){
      const float3d3 & prev=v[i-1];
      const float3d3 & cur=v[i];
      float3d prevx=prev.x,prevy=prev.y,curx=cur.x,cury=cur.y,m=cur.z;
      if (prevx==curx){
	if (x==curx && (y-prevy)*(cury-y)>0) // on vertical edge
	  return false;
	continue;
      }
      if (x==prevx) 
	continue;
      if ((x-prevx)*(curx-x)<0)
	continue;
      //float3d Y=cury+m*(x-curx);
      float3d Y=cury+(cur.y-prev.y)/(cur.x-prev.x)*(x-curx);
      if (Y>=y)
	++n; 
    }
    if (n%2)
      return true;
    return false;
  }
#else
  bool inside(const vector<float3d3> & v,float3d x,float3d y){
    int n=0;
    for (int i=1;i<v.size();++i){
      float3d3 prev=v[i-1],cur=v[i];
      float3d prevx=prev.x,prevy=prev.y,curx=cur.x,cury=cur.y,m=cur.z;
      if (prevx==curx){
	if (prevx!=x) continue;
	if (y==cury || (y-prevy)*(cury-y)>0)
	  ++n;
	continue;
      }
      if (x==prevx || (x-prevx)*(curx-x)<0)
	continue;
      float3d Y=cury+m*(x-curx);
      if (Y>=y)
	++n; 
    }
    if (n%2)
      return true;
    return false;
  }
#endif
  
  // intersect plane x-y=xy with line m+t*v
  // m.x+t*v.x-m.y-t*v.y=-xy
  float3d intersect(const float3d3 & m,const float3d3 & v,float3d xy){
    return (-xy+m.y-m.x)/(v.x-v.y);
  }

  bool get_colors(gen attr,int & upcolor,int & downcolor,int & downupcolor,int & downdowncolor){
    if (attr.is_symb_of_sommet(at_pnt)){
      attr=attr[1];
    }
    if (attr.type==_INT_ && (attr.val & 0xffff)!=0){
      upcolor=attr.val &0xffff;
      int color=rgb565to888(upcolor);
      int r=(color&0xff0000)>>16,g=(color & 0xff00)>>8,b=192;
      r >>= 2;
      g >>= 2;
      downcolor=rgb888to565((r<<16)|(g<<8)|b);
      r >>= 1;
      g >>= 1;
      downupcolor=rgb888to565((r<<16)|(g<<8)|b);
      r >>= 2;
      g >>= 2;
      downdowncolor=rgb888to565((r<<16)|(g<<8)|b);
    }
    if (attr.type==_INT_)
      return attr.val & 0x40000000;
    return false;
  }

#define ABC3D

  // 2d coordinates of m+t*v
  void grmtv2ij(const Graph2d & gr,const float3d3 & m,const float3d3 & v,float3d t,int & i,int & j){
    float3d x=m.x+t*v.x;
    float3d y=m.y+t*v.y;
    float3d z=m.z+t*v.z;
    gr.XYZ2ij(float3d3(x,y,z),i,j);
  }    
  
  // hpersurface encoded as a matrix
  // with lines containing 3 coordinates per point
  const int width3d=340;//LCD_WIDTH_PX;

  // load in ram, does not work from rom
  int arc_en_ciel(int k,float3d intensite){
    int r,g,b;
    k = (k+126)%126;
    if (k<21){
      r=251; g=0; b=12*k;
    }
    if (k>=21 && k<42){
      r=251-(12*(k-21)); g=0; b=251;
    } 
    if (k>=42 && k<63){
      r=0; g=(k-42)*12; b=251;
    } 
    if (k>=63 && k<84){
      r=0; g=251; b=251-(k-63)*12;
    } 
    if (k>=84 && k<105){
      r=(k-84)*12; g=251; b=0;
    } 
    if (k>=105 && k<126){
      r=251; g=251-(k-105)*12; b=0;
    }
    r*=intensite; g*=intensite; b*=intensite;
    //if (r==0) r=8;
    return (((r*32)/256)<<11) | (((g*64)/256)<<5) | (b*32/256);
  }

  // not initialized if const: see https://www.planet-casio.com/Fr/forums/topic15269-8-khicas-add-in-calcul-formel-pour-graph-90e-et-35eii.html#selform2 https://gitea.planet-casio.com/Lephenixnoir/gint/src/commit/4f9b141b79dfc2de10f6810329cc27cac2406eb6/fxcg50.ld#L46-L48, https://gitea.planet-casio.com/Lephenixnoir/gint/src/commit/4f9b141b79dfc2de10f6810329cc27cac2406eb6/src/kernel/start.c#L187, 
  //const
  int4 tabcolorcplx[]={
    int4(63488,47104,30720,14336),
    int4(63489,47105,30720,14336),
    int4(63491,47106,30721,14336),
    int4(63492,47107,30722,14337),
    int4(63494,47108,30723,14337),
    int4(63495,47109,30723,14337),
    int4(63497,47110,30724,14338),
    int4(63498,47111,30725,14338),
    int4(63500,47113,30726,14339),
    int4(63501,47114,30726,14339),
    int4(63503,47115,30727,14339),
    int4(63504,47116,30728,14340),
    int4(63506,47117,30729,14340),
    int4(63507,47118,30729,14340),
    int4(63509,47119,30730,14341),
    int4(63510,47120,30731,14341),
    int4(63512,47122,30732,14342),
    int4(63513,47123,30732,14342),
    int4(63515,47124,30733,14342),
    int4(63516,47125,30734,14343),
    int4(63518,47126,30735,14343),
    int4(63519,47127,30735,14343),
    int4(59423,45079,28687,14343),
    int4(57375,43031,28687,14343),
    int4(53279,40983,26639,12295),
    int4(51231,38935,24591,12295),
    int4(47135,34839,22543,10247),
    int4(45087,32791,22543,10247),
    int4(40991,30743,20495,10247),
    int4(38943,28695,18447,8199),
    int4(34847,26647,16399,8199),
    int4(32799,24599,16399,8199),
    int4(28703,22551,14351,6151),
    int4(26655,20503,12303,6151),
    int4(22559,16407,10255,4103),
    int4(20511,14359,10255,4103),
    int4(16415,12311,8207,4103),
    int4(14367,10263,6159,2055),
    int4(10271,8215,4111,2055),
    int4(8223,6167,4111,2055),
    int4(4127,4119,2063,7),
    int4(2079,2071,15,7),
    int4(2079,2071,2063,2055),
    int4(2175,2135,2095,2055),
    int4(2271,2199,2159,2087),
    int4(2367,2263,2191,2119),
    int4(2463,2359,2255,2151),
    int4(2559,2423,2287,2151),
    int4(2655,2487,2351,2183),
    int4(2751,2551,2383,2215),
    int4(2847,2647,2447,2247),
    int4(2943,2711,2479,2247),
    int4(3039,2775,2543,2279),
    int4(3135,2839,2575,2311),
    int4(3231,2935,2639,2343),
    int4(3327,2999,2671,2343),
    int4(3423,3063,2735,2375),
    int4(3519,3127,2767,2407),
    int4(3615,3223,2831,2439),
    int4(3711,3287,2863,2439),
    int4(3807,3351,2927,2471),
    int4(3903,3415,2959,2503),
    int4(3999,3511,3023,2535),
    int4(4063,3575,3055,2535),
    int4(4061,3574,3054,2535),
    int4(4060,3573,3054,2535),
    int4(4058,3572,3053,2534),
    int4(4057,3571,3052,2534),
    int4(4055,3569,3051,2533),
    int4(4054,3568,3051,2533),
    int4(4052,3567,3050,2533),
    int4(4051,3566,3049,2532),
    int4(4049,3565,3048,2532),
    int4(4048,3564,3048,2532),
    int4(4046,3563,3047,2531),
    int4(4045,3562,3046,2531),
    int4(4043,3560,3045,2530),
    int4(4042,3559,3045,2530),
    int4(4040,3558,3044,2530),
    int4(4039,3557,3043,2529),
    int4(4037,3556,3042,2529),
    int4(4036,3555,3042,2529),
    int4(4034,3554,3041,2528),
    int4(4033,3553,3040,2528),
    int4(4032,3552,3040,2528),
    int4(4032,3552,992,480),
    int4(8128,5600,3040,480),
    int4(10176,7648,5088,2528),
    int4(14272,9696,7136,2528),
    int4(16320,11744,7136,2528),
    int4(20416,13792,9184,4576),
    int4(22464,15840,11232,4576),
    int4(26560,19936,13280,6624),
    int4(28608,21984,13280,6624),
    int4(32704,24032,15328,6624),
    int4(34752,26080,17376,8672),
    int4(38848,28128,19424,8672),
    int4(40896,30176,19424,8672),
    int4(44992,32224,21472,10720),
    int4(47040,34272,23520,10720),
    int4(51136,38368,25568,12768),
    int4(53184,40416,25568,12768),
    int4(57280,42464,27616,12768),
    int4(59328,44512,29664,14816),
    int4(63424,46560,31712,14816),
    int4(65472,48608,31712,14816),
    int4(65376,48512,31648,14784),
    int4(65280,48448,31616,14784),
    int4(65184,48384,31552,14752),
    int4(65088,48320,31520,14720),
    int4(64992,48224,31456,14688),
    int4(64896,48160,31424,14688),
    int4(64800,48096,31360,14656),
    int4(64704,48032,31328,14624),
    int4(64608,47936,31264,14592),
    int4(64512,47872,31232,14592),
    int4(64416,47808,31168,14560),
    int4(64320,47744,31136,14528),
    int4(64224,47648,31072,14496),
    int4(64128,47584,31040,14496),
    int4(64032,47520,30976,14464),
    int4(63936,47456,30944,14432),
    int4(63840,47360,30880,14400),
    int4(63744,47296,30848,14400),
    int4(63648,47232,30784,14368),
    int4(63552,47168,30752,14336),
 };
  
  struct hypertriangle_t {
    const int4 * colorptr; // hypersurface color 
    float3d xmin,xmax,ymin,ymax; // minmax values intersection with plane y-x=Cte
    float3d a,b,c; // plane equation of triangle
    float3d zG; // altitude for gravity center 
  }  ; // data struct for hypesurface triangulation cache

    #define HYPERQUAD
#ifdef HYPERQUAD
  
  void compute(float3d yx,float3d3 * cur,hypertriangle_t & res){
    float3d xmin=1e307,xmax=-1e307,ymin=1e307,ymax=-1e307;
    for (int l=0;l<4;++l){
      int prev=l==0?3:l-1;
      float3d3 & d3=cur[prev];
      float3d x0=d3.x,y0=d3.y,x1=cur[l].x,y1=cur[l].y;
      float3d yx0=y0-x0,yx1=y1-x1,m=yx1-yx0;
      if (m==0){
	if (yx==yx1){
	  if (x0>xmax) xmax=x0; if (x0<xmin) xmin=x0;
	  if (x1>xmax) xmax=x1; if (x1<xmin) xmin=x1;
	  if (y0>ymax) ymax=y0; if (y0<ymin) ymin=y0;
	  if (y1>ymax) ymax=y1; if (y1<ymin) ymin=y1;
	}
	continue;
      }
      float3d t=(yx-yx0)/m;
      if (t>=0 && t<=1){
	float3d X=x0+t*(x1-x0),Y=y0+t*(y1-y0);
	if (X>xmax) xmax=X; if (X<xmin) xmin=X;
	if (Y>ymax) ymax=Y; if (Y<ymin) ymin=Y;
      }
    }
    res.zG=(cur[0].z+cur[1].z+cur[2].z+cur[3].z)/4;
    res.xmin=xmin; res.xmax=xmax; res.ymin=ymin; res.ymax=ymax;
    find_abc(cur[0].x,cur[1].x,cur[2].x,
	     cur[0].y,cur[1].y,cur[2].y,
	     cur[0].z,cur[1].z,cur[2].z,
	     res.a,res.b,res.c);
  }

  
#else
  void compute(float3d yx,float3d3 * cur,hypertriangle_t & res){
    res.zG=(cur[0].z+cur[1].z+cur[2].z)/3;
    float3d xmin=1e307,xmax=-1e307,ymin=1e307,ymax=-1e307;
    for (int l=0;l<3;++l){
      float3d3 & d3=cur[l?l-1:2];
      float3d x0=d3.x,y0=d3.y,x1=cur[l].x,y1=cur[l].y;
      float3d yx0=y0-x0,yx1=y1-x1,m=yx1-yx0;
      if (m==0){
	if (yx==yx1){
	  if (x0>xmax) xmax=x0; if (x0<xmin) xmin=x0;
	  if (x1>xmax) xmax=x1; if (x1<xmin) xmin=x1;
	  if (y0>ymax) ymax=y0; if (y0<ymin) ymin=y0;
	  if (y1>ymax) ymax=y1; if (y1<ymin) ymin=y1;
	}
	continue;
      }
      float3d t=(yx-yx0)/m;
      if (t>=0 && t<=1){
	float3d X=x0+t*(x1-x0),Y=y0+t*(y1-y0);
	if (X>xmax) xmax=X; if (X<xmin) xmin=X;
	if (Y>ymax) ymax=Y; if (Y<ymin) ymin=Y;
      }
    }
    res.xmin=xmin; res.xmax=xmax; res.ymin=ymin; res.ymax=ymax;
    find_abc(cur[0].x,cur[1].x,cur[2].x,
	     cur[0].y,cur[1].y,cur[2].y,
	     cur[0].z,cur[1].z,cur[2].z,
	     res.a,res.b,res.c);
  }
#endif

  void update_hypertri(const vector<hypertriangle_t> & hypertriangles,float3d x,float3d y,
		       bool & found,bool &found2,
		       float3d3 & curabc1,float3d & curz1,
		       float3d3 & curabc2,float3d & curz2,
		       int & upcolor,int & downcolor,int & downupcolor,int & downdowncolor){
    vector<hypertriangle_t>::const_iterator it=hypertriangles.begin(),itend=hypertriangles.end();
    for (;it!=itend;++it){
      if (x<it->xmin){
	++it;
	if (it==itend) break;
	if (x<it->xmin){
	  ++it;
	  if (it==itend) break;
	  if (x<it->xmin){
	    ++it;
	    if (it==itend) break;
	  }
	}
      }
      else if (x>it->xmax){
	++it;
	if (it==itend) break;
	if (x>it->xmax){
	  ++it;
	  if (it==itend) break;
	  if (x>it->xmax){
	    ++it;
	    if (it==itend) break;
	  }
	}
      }
      const hypertriangle_t & cur=*it;
      if (x<cur.xmin || x>cur.xmax || y<cur.ymin || y>cur.ymax)
	continue;
      if (!found || cur.zG>curz1){
	if (found){
	  found2=true;
	  curabc2=curabc1;
	  curz2=curz1;
	}
	found=true;
	curabc1.x=cur.a; curabc1.y=cur.b; curabc1.z=cur.c;
	curz1=cur.zG;
	upcolor=cur.colorptr->u; downcolor=cur.colorptr->d;
	//confirm(("update "+print_INT_(upcolor)+" "+print_INT_(downcolor)).c_str(),"");
	continue;
      }
      if (cur.zG>curz2){
	found2=true;
	curabc2.x=cur.a; curabc2.y=cur.b; curabc2.z=cur.c;
	curz2=cur.zG;
	downupcolor=cur.colorptr->du; downdowncolor=cur.colorptr->dd;
	continue;
      }
    } // end loop on k
  }
  
  void Graph2d::add_entry(int n){
    if (!hp)
      return;
    if (n==-1 || n>=symbolic_instructions.size()){
      symbolic_instructions.push_back(0);
      n=symbolic_instructions.size();
    }
    hp->add_entry(n);
  }

  struct float2 {
    float f,a;
  } ;
  float3d absarg(const gen & g,float3d & argcolor){
    if (g.type==_DOUBLE_){
      float3d d=g._DOUBLE_val;
      if (d>=0){ argcolor=0;  return d; }
      argcolor=M_PI; return -d;
    }
    float3d x=g._CPLXptr->_DOUBLE_val,y=(g._CPLXptr+1)->_DOUBLE_val;
    argcolor=std::atan2(y,x);
    float3d n=std::sqrt(x*x+y*y); // will be encoded in a float, no overflow care
    return n;
  }

bool discard(Graph2d * gr,float3d x,float3d y,float3d z){
  float3d X,Y,Z,f=0.1;
  do_transform(gr->invtransform,x,y,z,X,Y,Z);
  float3d dX=f*(gr->window_xmax-gr->window_xmin);
  if (X<gr->window_xmin-dX || X>gr->window_xmax+dX)
    return true;
  float3d dY=f*(gr->window_ymax-gr->window_ymin);
  if (Y<gr->window_ymin-dY || Y>gr->window_ymax+dY)
    return true;
  float3d dZ=f*(gr->window_zmax-gr->window_zmin);
  if (Z<gr->window_zmin-dZ || Z>gr->window_zmax+dZ)
    return true;
  return false;
}


  bool Graph2d::glsurface(int w,int h,int lcdz,GIAC_CONTEXT,
			  int upcolor_,int downcolor_,int downupcolor_,int downdowncolor_)  {
    if (g.type==_INT_ || (g.type==_VECT && g._VECTptr->size()==1 && g._VECTptr->front().type==_INT_)){
      drawRectangle(0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX,_BLACK);
      os_sync_screen();
      return true;
    }
    if (w>9) w=9; if (w<1) w=1;
    if (h>9) h=9; if (h<1) h=1;
    // save zmin/zmax on the stack (4K required)
    const int jmintabsize=512;
    short int *jmintab=(short int *)alloca(jmintabsize*sizeof(short int)), * jmaxtab=(short int *)alloca(jmintabsize*sizeof(short int)); // assumes width3d<=jmintabsize
    for (int i=0;i<jmintabsize;++i){
      jmintab[i]=LCD_HEIGHT_PX;
      jmaxtab[i]=0;
    }
    vecteur attrv(gen2vecteur(g));
    std::vector< std::vector< vector<float3d> >::const_iterator > hypv; // 3 iterateurs per hypersurface
    int upcolor,downcolor,downupcolor,downdowncolor;
    for (int i=0;i<attrv.size();++i){
      gen attr=attrv[i];
      upcolor=upcolor_;downcolor=downcolor_;downupcolor=downupcolor_;downdowncolor=downdowncolor_;
      get_colors(attrv[i],upcolor,downcolor,downupcolor,downdowncolor);
    }
    for (int i=0;i<surfacev.size();++i){
      hypv.push_back(surfacev[i].begin());
      hypv.push_back(surfacev[i].end());
    }
    //cout << "surf " << hypv.size() << " w=" << w << " lcdz=" << lcdz << " " << upcolor_ << " " << downcolor_ << " " << downupcolor_ << " " << downdowncolor_ << "\n";  
    int horiz=width3d/2,vert=horiz/2;//LCD_HEIGHT_PX/2;
    int imin=Ai,imax=Ai,itmp;
    // 12 segments from cube visualization
    float3d segments_x1[12]={Ai,Bi,Ei,Fi,Ai,Bi,Ci,Di,Ai,Ci,Ei,Gi};
    float3d segments_x2[12]={Ci,Di,Gi,Hi,Ei,Fi,Gi,Hi,Bi,Di,Fi,Hi};
    float3d segments_y1[12]={Aj,Bj,Ej,Fj,Aj,Bj,Cj,Dj,Aj,Cj,Ej,Gj};
    float3d segments_y2[12]={Cj,Dj,Gj,Hj,Ej,Fj,Gj,Hj,Bj,Dj,Fj,Hj};
    float3d segments_m[12];
    for (int i=0;i<12;++i){
      segments_m[i]=(segments_y2[i]-segments_y1[i])/(segments_x2[i]-segments_x1[i]);
      itmp=segments_x1[i];
      if (itmp<imin) imin=itmp; if (itmp>imax) imax=itmp;
      itmp=segments_x2[i];
      if (itmp<imin) imin=itmp; if (itmp>imax) imax=itmp;      
    }
    float3d xmin=-1,ymin=-1,xmax=1,ymax=1,xscale=0.6*(xmax-xmin)/horiz,yscale=0.6*(ymax-ymin)/vert,x,y,z,xc=(xmin+xmax)/2,yc=(ymin+ymax)/2;
    drawRectangle(0,0,imin,LCD_HEIGHT_PX,COLOR_BLACK); // clear    
    drawRectangle(imax,0,LCD_WIDTH_PX-imax,LCD_HEIGHT_PX,COLOR_BLACK); // clear
    DisplayStatusArea();
    if (solid3d) os_sync_screen();
    int count=0;
    vector<int> polyedrei; polyedrei.reserve(polyedrev.size()); // cache for polyedres polygons edges
    vector<float3d> polyedrexmin,polyedrexmax,polyedreymin,polyedreymax;
    polyedrexmin.reserve(polyedrev.size());polyedrexmax.reserve(polyedrev.size());
    polyedreymin.reserve(polyedrev.size());polyedreymax.reserve(polyedrev.size());
    vector<hypertriangle_t> hypertriangles;
    bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
    for (int i=imin-horiz;i<imax-horiz;i+=w,++count){
    //for (int i=-horiz;i<horiz;i+=w){
#if defined FXCG
      if (solid3d && count%4==3)
	os_sync_screen();
      control_c();
      if (ctrl_c || interrupted){
	// w+=2; h+=2;
	ctrl_c=interrupted=false;
	return true;
      }
#endif
      if (!is_emulator){
	int key,ret=in_ckgetkey(&key,false,0,0,0,0);
	if (ret>=0){
	  if (key==KEY_CTRL_DEL)
	    return true;
	  if (key==KEY_CTRL_OPTN){
	    w++; h++;
	  }
	}
      }
      drawRectangle(i+horiz,STATUS_AREA_PX,w,LCD_HEIGHT_PX-STATUS_AREA_PX,COLOR_BLACK); // clear
      // find min and max values for j using vertical intersections of line x=i
      // with segments
      int ih=i+horiz+w/2,jmin=RAND_MAX,jmax=-RAND_MAX;
      for (int k=0;k<12;++k){
	if ( !is_inf(segments_m[k]) && (ih-segments_x1[k])*(segments_x2[k]-ih)>=0 ){
	  float3d y=segments_y1[k]+segments_m[k]*(ih-segments_x1[k]);
	  if (y<=jmin)
	    jmin=std::floor(y);
	  if (y>=jmax)
	    jmax=std::ceil(y);
	}
      }
      // cout << jmin << " " << jmax << "\n";
      if (jmin>jmax) continue;
      if (jmin<0) jmin=0;
      if (jmax>LCD_HEIGHT_PX) jmax=LCD_HEIGHT_PX;
      jmin -= LCD_HEIGHT_PX/2;
      jmax -= LCD_HEIGHT_PX/2;      
      // optimization caches for this vertical
      float3d yx=2*xscale*(i+(w-1)/2.0)+yc-xc;
      // poledrev indices for yx, and xmin/xmax/ymin/ymax values
      // (xmin/xmax should be enough, except limit cases)
      polyedrei.clear(); polyedrexmin.clear(); polyedrexmax.clear(); polyedreymin.clear(); polyedreymax.clear();
      for (int k=0;k<int(polyedrev.size());++k){
	float3d facemin=polyedre_xyminmax[2*k],facemax=polyedre_xyminmax[2*k+1];
	if (yx<facemin || yx>facemax)
	  continue;
	polyedrei.push_back(k);
	vector<float3d3> & cur=polyedrev[k];
	float3d xmin=1e307,xmax=-1e307,ymin=1e307,ymax=-1e307;
	for (int l=0;l<int(cur.size());++l){
	  float3d3 & d3=cur[l?l-1:cur.size()-1];
	  float3d x0=d3.x,y0=d3.y,x1=cur[l].x,y1=cur[l].y;
	  float3d yx0=y0-x0,yx1=y1-x1,m=yx1-yx0;
	  if (m==0){
	    if (yx==yx1){
	      if (x0>xmax) xmax=x0; if (x0<xmin) xmin=x0;
	      if (x1>xmax) xmax=x1; if (x1<xmin) xmin=x1;
	      if (y0>ymax) ymax=y0; if (y0<ymin) ymin=y0;
	      if (y1>ymax) ymax=y1; if (y1<ymin) ymin=y1;
	    }
	    continue;
	  }
	  float3d t=(yx-yx0)/m;
	  if (t>=0 && t<=1){
	    float3d X=x0+t*(x1-x0),Y=y0+t*(y1-y0);
	    if (X>xmax) xmax=X; if (X<xmin) xmin=X;
	    if (Y>ymax) ymax=Y; if (Y<ymin) ymin=Y;
	  }
	}
	polyedrexmin.push_back(xmin);
	polyedrexmax.push_back(xmax);
	polyedreymin.push_back(ymin);
	polyedreymax.push_back(ymax);
      }
      // hypersurfaces: find triangles
      hypertriangles.clear();
      float3d hyperxymax=-1e307,hyperxymin=1e307;
      float3d3 tri[4];
#if 1
      if (tabcolorcplx[0].u==0){
	for (int i=0;i<126;i++){
	  tabcolorcplx[i].u=arc_en_ciel(i,1);
	  tabcolorcplx[i].d=arc_en_ciel(i,.75);
	  tabcolorcplx[i].du=arc_en_ciel(i,.5);
	  tabcolorcplx[i].dd=arc_en_ciel(i,.25);
	}
      }
#endif
      for (int k=0;k<int(hypv.size());k+=2){
	bool cplx=hyp_color[k].u<0 || hyp_color[k].d<0 || hyp_color[k].du<0 || hyp_color[k].dd<0;
	vector< vector<float3d> >::const_iterator sbeg=hypv[k],send=hypv[k+1],sprec,scur;
	vector<float3d>::const_iterator itprec,itcur,itprecend;
	for (sprec=sbeg,scur=sprec+1;scur<send;++sprec,++scur){
	  itprec=sprec->begin(); 
	  itprecend=sprec->end();
	  itcur=scur->begin();
	  float3d yx1,yx2=*(itprec+1)-*itprec,yx3,yx4=*(itcur+1)-*itcur;
	  for (itprec+=3,itcur+=3;itprec<itprecend;itprec+=3,itcur+=3){
	    yx1=yx2;
	    yx2=*(itprec+1)-*itprec;
	    yx3=yx4;
	    yx4=*(itcur+1)-*itcur;
	    if (yx<yx1 && yx<yx2 && yx<yx3 && yx<yx4){
	      for (;;){
		// per iteration: 2 incr, 1 test, 2 read, 2 comp, && , test
		itprec+=3;itcur+=3;
		if (itprec<itprecend && yx<(yx2=*(itprec+1)-*itprec) && yx<(yx4=*(itcur+1)-*itcur)){
		  itprec+=3;itcur+=3;
		  if (itprec<itprecend && yx<(yx2=*(itprec+1)-*itprec) && yx<(yx4=*(itcur+1)-*itcur)){
		    itprec+=3;itcur+=3;
		    if (itprec<itprecend && yx<(yx2=*(itprec+1)-*itprec) && yx<(yx4=*(itcur+1)-*itcur)){
		      itprec+=3;itcur+=3;
		      if (itprec<itprecend && yx<(yx2=*(itprec+1)-*itprec) && yx<(yx4=*(itcur+1)-*itcur)){
			continue;
		      }
		    }
		  }
		}
		break;
	      }
	      if (yx<yx2 && yx<yx4) continue;
	    } // end yx<yxk
	    else if (yx>yx1 && yx>yx2 && yx>yx3 && yx>yx4){
	      for (;;){
		// per iteration: 2 incr, 1 test, 2 read, 2 comp, && , test
		itprec+=3;itcur+=3;
		if (itprec<itprecend && yx>(yx2=*(itprec+1)-*itprec) && yx>(yx4=*(itcur+1)-*itcur)){
		  itprec+=3;itcur+=3;
		  if (itprec<itprecend && yx>(yx2=*(itprec+1)-*itprec) && yx>(yx4=*(itcur+1)-*itcur)){
		    itprec+=3;itcur+=3;
		    if (itprec<itprecend && yx>(yx2=*(itprec+1)-*itprec) && yx>(yx4=*(itcur+1)-*itcur)){
		      itprec+=3;itcur+=3;
		      if (itprec<itprecend && yx>(yx2=*(itprec+1)-*itprec) && yx>(yx4=*(itcur+1)-*itcur)){
			continue;
		      }
		    }
		  }
		}
		break;
	      }
	      if (yx>yx2 && yx>yx4) continue;
	    }
	    // found one quad intersecting plane
	    float3d x1=*(itprec-3),x2=*(itprec),x3=*(itcur-3),x4=*(itcur);
	    float3d y1=*(itprec-2),y2=*(itprec+1),y3=*(itcur-2),y4=*(itcur+1);
	    float3d z1=*(itprec-1),z2=*(itprec+2),z3=*(itcur-1),z4=*(itcur+2);
	    float3d a1,a2,a3,a4;
	    float2 tmp;
	    if (cplx){
	      tmp= *(float2 *) &z1;
	      a1 = tmp.a;
	      z1=tmp.f;
	      //confirm(print_FLOAT3D_(a1,3).c_str(),"");
	      tmp= *(float2 *) &z2;
	      z2 = tmp.f;
	      tmp= *(float2 *) &z3;
	      z3 = tmp.f;
	      tmp= *(float2 *) &z4;
	      z4 = tmp.f;
	    }
	    yx1=y1-x1; yx2=y2-x2; yx3=y3-x3; yx4=y4-x4;
#ifdef HYPERQUAD
            if (precision<=3 && (discard(this,x1,y1,z1) || discard(this,x2,y2,z2) || discard(this,x3,y3,z3)) )
              continue;
	    tri[0]=float3d3(x1,y1,z1);
	    tri[1]=float3d3(x2,y2,z2);
	    tri[2]=float3d3(x4,y4,z4);
	    tri[3]=float3d3(x3,y3,z3);
	    float3d x123=(x1+x2+x3+x4)/4,y123=(y1+y2+y3+y4)/4,z123=(z1+z2+z3+z4)/4,X,Y,Z;
	    float3d xy123=x123+y123;
	    if (xy123<hyperxymin) hyperxymin=xy123;
	    if (xy123>hyperxymax) hyperxymax=xy123;
	    do_transform(invtransform,x123,y123,z123,X,Y,Z);
	    if (Z>=window_zmin && Z<=window_zmax && X>=window_xmin && X<=window_xmax && Y>=window_ymin && Y<=window_ymax){
	      hypertriangle_t res;
	      if (cplx){
		int idx=(a1+M_PI)*sizeof(tabcolorcplx)/(sizeof(int4)*2*M_PI);
		if (idx<0 || idx >=sizeof(tabcolorcplx)/(sizeof(int4)))
		  idx=0;
		//confirm((print_INT_(idx)+" "+print_INT_(tabcolorcplx[idx].u)).c_str(),"");
		res.colorptr=&tabcolorcplx[idx];
	      }
	      else
		res.colorptr=&hyp_color[k];
	      compute(yx,tri,res);
	      hypertriangles.push_back(res);
	    }
#else // HYPERQUAD
	    tri[1]=float3d3(x2,y2,z2);
	    tri[2]=float3d3(x3,y3,z3);
	    if ( (yx>yx1 && yx>yx2 && yx>yx3) ||
		 (yx<yx1 && yx<yx2 && yx<yx3) )
	      ; // not intersecting
	    else {
	      float3d x123=(x1+x2+x3)/3,y123=(y1+y2+y3)/3,z123=(z1+z2+z3)/3,X,Y,Z;
	      float3d xy123=x123+y123;
	      if (xy123<hyperxymin) hyperxymin=xy123;
	      if (xy123>hyperxymax) hyperxymax=xy123;
	      do_transform(invtransform,x123,y123,z123,X,Y,Z);
	      if (Z>=window_zmin && Z<=window_zmax && X>=window_xmin && X<=window_xmax && Y>=window_ymin && Y<=window_ymax ){
		tri[0]=float3d3(x1,y1,z1);
		hypertriangle_t res;
		if (cplx){
		  int idx=(a1+M_PI)*sizeof(tabcolorcplx)/(sizeof(int4)*2*M_PI);
		  if (idx<0 || idx >=sizeof(tabcolorcplx)/(sizeof(int4)))
		    idx=0;
		  res.colorptr=&tabcolorcplx[idx];
		}
		else
		  res.colorptr=&hyp_color[k];
		compute(yx,tri,res);
		hypertriangles.push_back(res);
	      }
	    }
	    if ( (yx>yx4 && yx>yx2 && yx>yx3) ||
		 (yx<yx4 && yx<yx2 && yx<yx3) )
	      ; // not intersecting
	    else {
	      float3d x423=(x4+x2+x3)/3,y423=(y4+y2+y3)/3,z423=(z4+z2+z3)/3,X,Y,Z;
	      float3d xy423=x423+y423;
	      if (xy423<hyperxymin) hyperxymin=xy423;
	      if (xy423>hyperxymax) hyperxymax=xy423;
	      do_transform(invtransform,x423,y423,z423,X,Y,Z);
	      if (Z>=window_zmin && Z<=window_zmax && X>=window_xmin && X<=window_xmax && Y>=window_ymin && Y<=window_ymax ){
		tri[0]=float3d3(x4,y4,z4);
		hypertriangle_t res;
		if (cplx){
		  int idx=(a1+M_PI)*sizeof(tabcolorcplx)/(sizeof(int4)*2*M_PI);
		  if (idx<0 || idx >=sizeof(tabcolorcplx)/(sizeof(int4)))
		    idx=0;
		  res.colorptr=&tabcolorcplx[idx];
		}
		else
		  res.colorptr=&hyp_color[k];		
		compute(yx,tri,res);
		hypertriangles.push_back(res);
	      }
	    }
#endif // HYPERQUAD
	  }
	}
      }
      vector<bool> spheres(sphere_centerv.size()); // is plane y-x=yx intersecting sphere
      for (int k=0;k<int(sphere_centerv.size());++k){
	const float3d3 & c=sphere_centerv[k];
	float3d xc=c.x,yc=c.y;
	float3d r=sphere_radiusv[k];
	const matrice & m=*sphere_quadraticv[k]._VECTptr;
	const vecteur & m0=*m[0]._VECTptr;
	const vecteur & m1=*m[1]._VECTptr;
	const vecteur & m2=*m[2]._VECTptr;
	float3d m00=m0[0]._DOUBLE_val,m01=m0[1]._DOUBLE_val,m02=m0[2]._DOUBLE_val,m11=m1[1]._DOUBLE_val,m12=m1[2]._DOUBLE_val,m22=m2[2]._DOUBLE_val;
	/* q0:=m00*x^2+2*m01*x*y+2*m02*x*z+m11*y^2+2*m12*y*z+m22*z^2; q:=subst(q0,[x,y],[x-xc,y-yc]);
	   a,b,c:=coeffs(q(y=yx+x)-r^2,x);
	   delta:=b^2-4*a*c; 
	   // if delta<0 for all z, there is no intersection
	   // delta is a second order polynomial in z, check discriminant
	   A,B,C:=coeffs(delta,z);
	   D:=B^2-4*A*C;  // if D<0 no intersection
	*/
	float3d A=4*m02*m02+4*m12*m12-4*m00*m22-8*m01*m22+8*m02*m12-4*m11*m22,
	  B=-8*m00*m12*xc+8*m00*m12*yc-8*m00*m12*yx+8*m01*m02*xc-8*m01*m02*yc+8*m01*m02*yx-8*m01*m12*xc+8*m01*m12*yc-8*m01*m12*yx+8*m02*m11*xc-8*m02*m11*yc+8*m02*m11*yx,
	  C=4*m01*m01*xc*xc+4*m01*m01*yc*yc+4*m01*m01*yx*yx-4*m00*m11*xc*xc-4*m00*m11*yc*yc-4*m00*m11*yx*yx-8*m01*m01*xc*yc+8*m01*m01*xc*yx-8*m01*m01*yc*yx+8*m00*m11*xc*yc-8*m00*m11*xc*yx+8*m00*m11*yc*yx+4*m00*r*r+8*m01*r*r+4*m11*r*r,
	  D=B*B-4*A*C;
	spheres[k]=D>=0;
      }
      float3d zmin[10]={220.220,220,220,220,220,220,220,220,220},
	zmax[10]={0,0,0,0,0,0,0,0,0,0},
	zmin2[10]={220.220,220,220,220,220,220,220,220,220},
	zmax2[10]={0,0,0,0,0,0,0,0,0,0}	; // initialize for these vertical lines
#ifdef ABC3D
      float3d3 curabc1,curabc2; 
      float3d curz1=-1e306,curz2=1e306;
#else
      float3d curx1,curx2,curx3,cury1,cury2,cury3,curz1=-1e306,curz2=-1e306,curz3=-1e306;
      float3d cur2x1,cur2x2,cur2x3,cur2y1,cur2y2,cur2y3,cur2z1=-1e306,cur2z2=-1e306,cur2z3=-1e306;
#endif
      int u,d,du,dd;
      // loop earlier if there are only hypersurfaces
      bool only_hypertri=true;
      for (int ki=0;ki<int(polyedrei.size());++ki){
	if (polyedrexmin[ki]<=polyedrexmax[ki]){ only_hypertri=false; break; }
      }
      for (int k=0;k<int(sphere_centerv.size());++k){
	if (spheres[k]){ only_hypertri=false; break; }
      }
      for (int k=0;k<int(plan_abcv.size());++k){
	if (plan_filled[k]){ only_hypertri=false; break; }
      }
      if (only_hypertri){
	if (hypertriangles.empty()) goto suite3d;
	int effjmax=(hyperxymax-xc-yc)/yscale/2.0,effjmin=(hyperxymin-xc-yc)/yscale/2.0;
	if (effjmax+1<jmax)
	  jmax=effjmax+1;
	if (effjmin-1>jmin)
	  jmin=effjmin-1;
	x = yscale*(jmax-(h-1)/2.0)-xscale*(i+(w-1)/2.0) + xc;
	y = yscale*(jmax-(h-1)/2.0)+xscale*(i+(w-1)/2.0) + yc;
	for (int j=jmax;j>=jmin;j-=h,x-=yscale*h,y-=yscale*h){
	  bool found=false,found2=false;
	  update_hypertri(hypertriangles,x,y,found,found2,curabc1,curz1,curabc2,curz2,upcolor,downcolor,downupcolor,downdowncolor);
	  if (!found) continue;
	  if (h==1 && w==1){
	    if (found2 && !hide2nd){
	      float3d dz=lcdz*(curabc2.x+curabc2.y)*yscale-1;
	      // if (y<ymin) continue;
	      float3d z = (curabc2.x*x+curabc2.y*y+curabc2.z);
	      z=LCD_HEIGHT_PX/2+j-lcdz*z;
	      glinter1(z,dz,
		       zmin2,zmax2,zmin[0],zmax[0],
		       ih,lcdz,
		       downupcolor,downdowncolor,diffusionz,diffusionz_limit,interval);
	    }
	    float3d dz=lcdz*(curabc1.x+curabc1.y)*yscale-1;
	    // if (y<ymin) continue;
	    float3d z = (curabc1.x*x+curabc1.y*y+curabc1.z);
	    z=LCD_HEIGHT_PX/2+j-lcdz*z;

	    glinter1(z,dz,
		     zmin,zmax,1e307,-1e307,
		     ih,lcdz,
		     upcolor,downcolor,diffusionz,diffusionz_limit,interval);
	  }
	  else {
	    if (found2 && !hide2nd)
	    glinter(curabc2.x,curabc2.y,curabc2.z,xscale,xc,yscale,yc,zmin2,zmax2,zmin[0],zmax[0],i,horiz,j,w,h,lcdz,downupcolor,downdowncolor,diffusionz,diffusionz_limit,interval);
	    glinter(curabc1.x,curabc1.y,curabc1.z,xscale,xc,yscale,yc,zmin,zmax,1e307,-1e307,i,horiz,j,w,h,lcdz,upcolor,downcolor,diffusionz,diffusionz_limit,interval);
	  }
	}
      } // end only_hypertri
      else {
	x = yscale*(jmax-(h-1)/2.0)-xscale*(i+(w-1)/2.0) + xc;
	y = yscale*(jmax-(h-1)/2.0)+xscale*(i+(w-1)/2.0) + yc;
	for (int j=jmax;j>=jmin;j-=h,x-=yscale*h,y-=yscale*h){
	  if (0 && i==-35 && j==-44)
	    u=0; // debug
	  // x = yscale*(j-(h-1)/2.0)-xscale*(i+(w-1)/2.0) + xc;
	  // y = yscale*(j-(h-1)/2.0)+xscale*(i+(w-1)/2.0) + yc;
	  bool found=false,found2=false;
	  if (x+y>=hyperxymin && x+y<=hyperxymax)
	    update_hypertri(hypertriangles,x,y,found,found2,curabc1,curz1,curabc2,curz2,upcolor,downcolor,downupcolor,downdowncolor);
	  for (int ki=0;ki<int(polyedrei.size());++ki){
	    int k=polyedrei[ki];
	    vector<float3d3> & cur=polyedrev[k];
	    if (
#if 1
		x>=polyedrexmin[ki] && x<=polyedrexmax[ki] && y>=polyedreymin[ki] && y<=polyedreymax[ki]
#else
		inside(cur,x,y)
#endif
		){
	      const float3d3 & abc=polyedre_abcv[k];
	      const int4 & color=polyedre_color[k];
	      // std::cout << k << " " << x << " " << y << " " << color.u << "\n";
	      float3d a=abc.x,b=abc.y,c=abc.z;
	      z=a*x+b*y+c;
	      bool is_clipped=polyedre_faceisclipped[k];
	      if (!is_clipped){
		float3d X,Y,Z;
		do_transform(invtransform,x,y,z,X,Y,Z);
		is_clipped=X>=window_xmin && X<=window_xmax && Y>=window_ymin && Y<=window_ymax && Z>=window_zmin && Z<=window_zmax;
	      }
	      if (is_clipped){
#ifdef ABC3D
		update12(found,found2,
			 a,b,c,z,
			 color.u,color.d,color.du,color.dd,
			 curabc1.x,curabc1.y,curabc1.z,curz1,
			 curabc2.x,curabc2.y,curabc2.z,curz2,
			 upcolor,downcolor,downupcolor,downdowncolor);
#else
		update12(found,found2,
			 x-.5,x-.5,x+1,y+0.866,y-0.866,y,z-.5*a+.866*b,z-.5*a-.866*b,z+a,color.u,color.d,color.du,color.dd,
			 curx1,curx2,curx3,cury1,cury2,cury3,curz1,curz2,curz3,
			 cur2x1,cur2x2,cur2x3,cur2y1,cur2y2,cur2y3,cur2z1,cur2z2,cur2z3,
			 upcolor,downcolor,downupcolor,downdowncolor);
#endif
	      }
	    } // end if inside(cur,x,y)
	  }
	  for (int k=0;k<int(sphere_centerv.size());++k){
	    if (!spheres[k]) continue;
	    const float3d3 & c=sphere_centerv[k];
	    float3d R=sphere_radiusv[k];
	    const matrice & m=*sphere_quadraticv[k]._VECTptr;
	    const vecteur & m0=*m[0]._VECTptr;
	    const vecteur & m1=*m[1]._VECTptr;
	    const vecteur & m2=*m[2]._VECTptr;
	    float3d v0=x-c.x,v1=y-c.y;
	    float3d a=m2[2]._DOUBLE_val,b=2*(m0[2]._DOUBLE_val*v0+m1[2]._DOUBLE_val*v1),C=(m0[0]._DOUBLE_val*v0+2*m0[1]._DOUBLE_val*v1)*v0+m1[1]._DOUBLE_val*v1*v1-R*R;
	    float3d delta=b*b-4*a*C;
	    if (delta<0)
	      continue;
	    const int4 & color=sphere_color[k];
	    delta=std::sqrt(delta);
	    float3d sol1,sol2;
	    if (b>0){
	      sol1=(-b-delta)/2/a;
	      sol2=2*C/(-b-delta); // (-b+delta)/2/a;
	    }
	    else {
	      sol1=2*C/(-b+delta);//(-b-delta)/2/a;
	      sol2=(-b+delta)/2/a;
	    }
	    float3d v2=sol1;
	    z=v2+c.z;
	    bool is_clipped=sphere_isclipped[k];
	    if (!is_clipped){
	      float3d X,Y,Z;
	      do_transform(invtransform,x,y,z,X,Y,Z);
	      is_clipped=X>=window_xmin && X<=window_xmax && Y>=window_ymin && Y<=window_ymax && Z>=window_zmin && Z<=window_zmax;
	    }
	    if (is_clipped){
	      float3d w0=v0*m0[0]._DOUBLE_val+v1*m1[0]._DOUBLE_val+v2*m2[0]._DOUBLE_val;
	      float3d w1=v0*m0[1]._DOUBLE_val+v1*m1[1]._DOUBLE_val+v2*m2[1]._DOUBLE_val;
	      float3d w2=v0*m0[2]._DOUBLE_val+v1*m1[2]._DOUBLE_val+v2*m2[2]._DOUBLE_val;
#ifdef ABC3D
	      float3d a=-w0/w2,b=-w1/w2,c=z-(a*x+b*y);
	      update12(found,found2,
		       a,b,c,z,
		       color.u,color.d,color.du,color.dd,
		       curabc1.x,curabc1.y,curabc1.z,curz1,
		       curabc2.x,curabc2.y,curabc2.z,curz2,
		       upcolor,downcolor,downupcolor,downdowncolor);
#else
	      update12(found,found2,
		       //x-w2,x,x,y,y,y-w2,z+w0,z,z+w1,
		       x-0.5,x-.5,x+1,y+.866,y-.866,y,z+.5*w0/w2-.866*w1/w2,z+.5*w0/w2+.866*w1/w2,z-w0/w2,
		       color.u,color.d,color.du,color.dd,
		       curx1,curx2,curx3,cury1,cury2,cury3,curz1,curz2,curz3,
		       cur2x1,cur2x2,cur2x3,cur2y1,cur2y2,cur2y3,cur2z1,cur2z2,cur2z3,
		       upcolor,downcolor,downupcolor,downdowncolor);
#endif
	    }
	    if (delta<=0) continue; // delta==0, twice the same point
	    v2=sol2;
	    z=v2+c.z;
	    is_clipped=sphere_isclipped[k];
	    if (!is_clipped){
	      float3d X,Y,Z;
	      do_transform(invtransform,x,y,z,X,Y,Z);
	      is_clipped=X>=window_xmin && X<=window_xmax && Y>=window_ymin && Y<=window_ymax && Z>=window_zmin && Z<=window_zmax;
	    }
	    if (is_clipped){
	      float3d w0=v0*m0[0]._DOUBLE_val+v1*m1[0]._DOUBLE_val+v2*m2[0]._DOUBLE_val;
	      float3d w1=v0*m0[1]._DOUBLE_val+v1*m1[1]._DOUBLE_val+v2*m2[1]._DOUBLE_val;
	      float3d w2=v0*m0[2]._DOUBLE_val+v1*m1[2]._DOUBLE_val+v2*m2[2]._DOUBLE_val;
#ifdef ABC3D
	      float3d a=-w0/w2,b=-w1/w2,c=z-(a*x+b*y);
	      update12(found,found2,
		       a,b,c,z,
		       color.u,color.d,color.du,color.dd,
		       curabc1.x,curabc1.y,curabc1.z,curz1,
		       curabc2.x,curabc2.y,curabc2.z,curz2,
		       upcolor,downcolor,downupcolor,downdowncolor);
#else
	      update12(found,found2,
		       //x-w2,x,x,y,y,y-w2,z+w0,z,z+w1,
		       x-0.5,x-.5,x+1,y+.866,y-.866,y,z+.5*w0/w2-.866*w1/w2,z+.5*w0/w2+.866*w1/w2,z-w0/w2,
		       color.u,color.d,color.du,color.dd,
		       curx1,curx2,curx3,cury1,cury2,cury3,curz1,curz2,curz3,
		       cur2x1,cur2x2,cur2x3,cur2y1,cur2y2,cur2y3,cur2z1,cur2z2,cur2z3,
		       upcolor,downcolor,downupcolor,downdowncolor);
#endif
	    }
	  } // end hypersphere loop
	  for (int k=0;k<int(plan_abcv.size());++k){
	    if (!plan_filled[k])
	      continue;
	    float3d3 abc=plan_abcv[k];
	    int4 color=plan_color[k];
	    // z=a*x+b*y+c
	    float3d z=abc.x*x+abc.y*y+abc.z,X,Y,Z;
	    do_transform(invtransform,x,y,z,X,Y,Z);
	    if (X>=window_xmin && X<=window_xmax && Y>=window_ymin && Y<=window_ymax && Z>=window_zmin && Z<=window_zmax)
#ifdef ABC3D
	      update12(found,found2,
		       abc.x,abc.y,abc.z,z,
		       color.u,color.d,color.du,color.dd,
		       curabc1.x,curabc1.y,curabc1.z,curz1,
		       curabc2.x,curabc2.y,curabc2.z,curz2,
		       upcolor,downcolor,downupcolor,downdowncolor);
#else
	    update12(found,found2,
		     x-1,x,x,y,y,y+1,z-abc.x,z,z+abc.y,color.u,color.d,color.du,color.dd,
		     curx1,curx2,curx3,cury1,cury2,cury3,curz1,curz2,curz3,
		     cur2x1,cur2x2,cur2x3,cur2y1,cur2y2,cur2y3,cur2z1,cur2z2,cur2z3,
		     upcolor,downcolor,downupcolor,downdowncolor);
#endif
	  } // end hyperplan loop
	  if (found){
#ifdef ABC3D
	    if (found2){
	      if (!hide2nd)
		glinter(curabc2.x,curabc2.y,curabc2.z,xscale,xc,yscale,yc,zmin2,zmax2,zmin[0],zmax[0],i,horiz,j,w,h,lcdz,downupcolor,downdowncolor,diffusionz,diffusionz_limit,interval);
	      glinter(curabc1.x,curabc1.y,curabc1.z,xscale,xc,yscale,yc,zmin,zmax,1e307,-1e307,i,horiz,j,w,h,lcdz,upcolor,downcolor,diffusionz,diffusionz_limit,interval);
	    }
	    else
	      glinter(curabc1.x,curabc1.y,curabc1.z,xscale,xc,yscale,yc,zmin,zmax,1e307,-1e307,i,horiz,j,w,h,lcdz,upcolor,downcolor,diffusionz,diffusionz_limit,interval);
#else
	    if (found2){
	      if (!hide2nd)
		glinter(cur2x1,cur2x2,cur2x3,cur2y1,cur2y2,cur2y3,cur2z1,cur2z2,cur2z3,xscale,xc,yscale,yc,zmin2,zmax2,zmin[0],zmax[0],i,horiz,j,w,h,lcdz,downupcolor,downdowncolor,diffusionz,diffusionz_limit,interval);
	      glinter(curx1,curx2,curx3,cury1,cury2,cury3,curz1,curz2,curz3,xscale,xc,yscale,yc,zmin,zmax,1e307,-1e307,i,horiz,j,w,h,lcdz,upcolor,downcolor,diffusionz,diffusionz_limit,interval);
	    }
	    else
	      glinter(curx1,curx2,curx3,cury1,cury2,cury3,curz1,curz2,curz3,xscale,xc,yscale,yc,zmin,zmax,1e307,-1e307,i,horiz,j,w,h,lcdz,upcolor,downcolor,diffusionz,diffusionz_limit,interval);
#endif
	  }
	  else {
	    //std::cout << "not inside " << i << " " << j << " " << x << " " << y << "\n";	      
	  }
	} // end pixel vertical loop on j
      } // end else only_hypertri
    suite3d:
      // update jmintab/jmaxtab
      if (i+horiz+w<jmintabsize){
	for (int I=0;I<w;++I){
	  jmintab[i+horiz+I]=zmin[I];
	  jmaxtab[i+horiz+I]=zmax[I];
	}
      }
      // now render line/segments/curves: find intersection with plane
      // y-x=yc-xc+xscale*(2*i+I), 0<=I<w
      for (int j=0;j<curvev.size();++j){
	vector<float3d3> & cur=curvev[j];
	int s=cur.size();
	if (s<2) continue;
	int4 color=curve_color[j];
	float3d xy=yc-xc+xscale*2*i;
	for (int l=0;l<s-1;++l){
	  float3d3 m=cur[l];
	  float3d3 n=cur[l+1];
	  float3d3 v(n.x-m.x,n.y-m.y,n.z-m.z);
	  if (fabs(v.y-v.x)<1e-4*(fabs(v.y)+fabs(v.x))){
	    v.y=v.x+1e-4*(fabs(v.y)+fabs(v.x));
	  }
	  float3d t1=intersect(m,v,xy);
	  if (t1<0 || t1>1)
	    continue;
	  float3d dt=2*xscale/(v.y-v.x); // di==1
	  float3d x1=m.x+t1*v.x;
	  float3d y1=m.y+t1*v.y;
	  float3d z1=m.z+t1*v.z;
	  float3d X1,Y1,Z1;
	  do_transform(invtransform,x1,y1,z1,X1,Y1,Z1);
	  if (X1<window_xmin || X1>window_xmax || Y1<window_ymin || Y1>window_ymax || Z1<window_zmin || Z1>window_zmax)
	    continue;
	  z1=LCD_HEIGHT_PX/2-lcdz*z1+(x1+y1)/2/yscale;
	  float3d dz=-dt*v.z*lcdz+dt*(v.x+v.y)/2/yscale;
	  int horiz=width3d/2;
	  for (int k=0;k<w;++k){
	    float3d Z1=z1,Z2=z1+dz;
	    z1=Z2;
	    if (Z1>Z2) std::swap(Z1,Z2);
	    // line [ (i+k,Z1), (i+k,Z2) ]
	    if (Z2<zmin[k]){
	      drawRectangle(i+horiz+k,Z1,1,std::ceil(Z2-Z1),color.u);
	      continue;
	    }
	    if (Z1>zmax[k]){
	      drawRectangle(i+horiz+k,Z1,1,std::ceil(Z2-Z1),color.d);
	      continue;
	    }
	    drawRectangle(i+horiz+k,Z1,1,std::ceil(Z2-Z1),color.du);
	    if (Z1<zmin[k])
	      drawRectangle(i+horiz+k,Z1,1,std::ceil(zmin[k]-Z1),color.u);
	    if (Z2>zmax[k])
	      drawRectangle(i+horiz+k,zmax[k],1,std::ceil(Z2-zmax[k]),color.d);
	  }
	} // end l loop on curve discretization
      } // end loop on curves
#ifdef OLD_LINE_RENDERING
      for (int j=0;j<linetypev.size();j++){
	float3d3 m=linev[2*j];
	float3d3 v=linev[2*j+1];
	int4 color=line_color[j];
	if (std::abs(v.y-v.x)<1e-4*(std::abs(v.y)+std::abs(v.x))){
	  v.y=v.x+1e-6*(std::abs(v.y)+std::abs(v.x));
	}
	float3d xy=yc-xc+xscale*2*i;
	float3d t1=intersect(m,v,xy);
	// for halfline, t1 must be >=0, for segments between 0 and 1
	if (linetypev[j]==_HALFLINE__VECT && t1<0)
	  continue;
	if (linetypev[j]==_GROUP__VECT && (t1<0 || t1>1))
	  continue;
	float3d dt=2*xscale/(v.y-v.x); // di==1
	float3d x1=m.x+t1*v.x;
	float3d y1=m.y+t1*v.y;
	float3d z1=m.z+t1*v.z;
	float3d X1,Y1,Z1;
	do_transform(invtransform,x1,y1,z1,X1,Y1,Z1);
	// int dbgi,dbgj; xyz2ij(float3d3(x1,y1,z1),dbgi,dbgj);
	/// float3d x2=x1+dt*v.x,y2=y1+dt*v.y,z2=z1+dt*v.z;
	if (X1<window_xmin || X1>window_xmax || Y1<window_ymin || Y1>window_ymax || Z1<window_zmin || Z1>window_zmax)
	  continue;
	z1=LCD_HEIGHT_PX/2-lcdz*z1+(x1+y1)/2/yscale;
	float3d dz=-dt*v.z*lcdz+dt*(v.x+v.y)/2/yscale;
	int horiz=width3d/2;
	for (int k=0;k<w;++k){
	  float3d Z1=z1,Z2=z1+dz;
	  z1=Z2;
	  if (Z1>Z2) std::swap(Z1,Z2);
	  // line [ (i+k,Z1), (i+k,Z2) ]
	  if (Z2<zmin[k]){
	    drawRectangle(i+horiz+k,Z1,1,std::ceil(Z2-Z1),color.u);
	    continue;
	  }
	  if (Z1>zmax[k]){
	    drawRectangle(i+horiz+k,Z1,1,std::ceil(Z2-Z1),color.d);
	    continue;
	  }
	  drawRectangle(i+horiz+k,Z1,1,std::ceil(Z2-Z1),color.du);
	  if (Z1<zmin[k])
	    drawRectangle(i+horiz+k,Z1,1,std::ceil(zmin[k]-Z1),color.u);
	  if (Z2>zmax[k])
	    drawRectangle(i+horiz+k,zmax[k],1,std::ceil(Z2-zmax[k]),color.d);	    
	}
      } // end lines rendering
#endif
      // points rendering
      for (int j=0;j<int(pointv.size());++j){
	const float3d3 & m=pointv[j];
	if (m.x<i+horiz || m.x>=i+horiz+w)
	  continue;
	const int4 & c=point_color[j];
	int k=m.x-i-horiz,color=-1;
	float3d mz=LCD_HEIGHT_PX/2-lcdz*m.z;
	float3d dz=(zmax[k]-zmin[k])*1e-3;
	if (mz>=zmax[k]-dz)
	  color=c.u;
	else if (mz<=zmin[k]+dz)
	  color=c.u; // c.d?
	else color=c.du;
	drawRectangle(m.x,m.y,3,3,color);
	if (points[j]){
	  // int dx=RAND_MAX+os_draw_string(-RAND_MAX,0,color,0,points[j],false); // fake print
	  int dx=os_draw_string(0,0,color,0,points[j],true); // fake print
	  os_draw_string(m.x-dx,m.y,color,0,points[j],false); 
	}
      } // end points rendering
    } // end pixel horizontal loop on i
#ifndef OLD_LINE_RENDERING
    // new line rendering
    for (int j=0;j<linetypev.size();j++){
      float3d mx,my,mz,vx,vy,vz;
      float3d3 m=linev[2*j];
      do_transform(invtransform,m.x,m.y,m.z,mx,my,mz);
      float3d3 v=linev[2*j+1];
      do_transform(invtransform,m.x+v.x,m.y+v.y,m.z+v.z,vx,vy,vz);
      vx -= mx; vy -= my; vz -= mz;
      int4 color=line_color[j];
      // find min/max parameter value for hidden/visible
      float3d tmax=1e306,tmin=-1e306;
      if (linetypev[j]==_HALFLINE__VECT){
	tmin=0;
      }
      if (linetypev[j]==_GROUP__VECT){
	tmin=0;
	tmax=1;
      }
      if (vx!=0){ // intersect with x=window_xmin and x=window_xmax
	// m.x+v.x*t=window_xmin/max
	float3d t1=(window_xmin-mx)/vx;
	float3d t2=(window_xmax-mx)/vx;
	if (t1>t2)
	  std::swap<float3d>(t1,t2);
	if (t1>tmin)
	  tmin=t1;
	if (t2<tmax)
	  tmax=t2;
      }
      if (vy!=0){ // intersect with y=window_ymin and y=window_ymax
	float3d t1=(window_ymin-my)/vy;
	float3d t2=(window_ymax-my)/vy;
	if (t1>t2)
	  std::swap<float3d>(t1,t2);
	if (t1>tmin)
	  tmin=t1;
	if (t2<tmax)
	  tmax=t2;
      }
      if (vz!=0){ // intersect with z=window_zmin and z=window_zmax
	float3d t1=(window_zmin-mz)/vz;
	float3d t2=(window_zmax-mz)/vz;
	if (t1>t2)
	  std::swap<float3d>(t1,t2);
	if (t1>tmin)
	  tmin=t1;
	if (t2<tmax)
	  tmax=t2;
      }
      // find which intersection we want
      bool usetmax=v.x+v.y==0?v.z>=0:v.x+v.y>0; 
      float3d tmin_=usetmax?tmin:tmax,
	tmax_=usetmax?tmin:tmax;
      for (int k=0;k<plan_abcv.size();++k){
	if (!plan_filled[k]) continue;
	// z >= z_plan=a*x+b*y+c where (x,y,z)=m+t*v
	// m.z+t*v.z >= a*m.x+t*a*v.x+b*m.y+t*b*v.y+c
	// t*(v.z-a*v.x-b.v.y) >= a*m.x+b*m.y+c-m.z
	float3d3 abc=plan_abcv[k];
	float3d A=v.z-abc.x*v.x-abc.y*v.y,B=abc.x*m.x+abc.y*m.y+abc.z-m.z;
	if (A==0) continue;
	float3d t=B/A;
	if (t<=tmin || t>=tmax)
	  continue;
	if (tmax_<t)
	  tmax_=t;
	if (tmin_>t)
	  tmin_=t;
      }
      for (int k=0;k<sphere_centerv.size();++k){
	// sphere interesect line 
	float3d3 c=sphere_centerv[k];
	float3d R=sphere_radiusv[k];
	int4 color=sphere_color[k];
	float3d cx,cy,cz;
	do_transform(invtransform,c.x,c.y,c.z,cx,cy,cz);
	// (mx+t*vx-cx)^2+(my+t*vy-cy)^2+(mz+t*vz-cz)^2=R^2
	float3d a=vx*vx+vy*vy+vz*vz,b=(vx*(mx-cx)+vy*(my-cy)+vz*(mz-cz)),C=(mx-cx)*(mx-cx)+(my-cy)*(my-cy)+(mz-cz)*(mz-cz)-R*R;
	float3d deltaprime=b*b-a*C;
	if (deltaprime>0){
	  deltaprime=std::sqrt(deltaprime);
	  float3d t1=(-b-deltaprime)/a,t2=(-b+deltaprime)/a;
	  if (t1>t2)
	    std::swap(t1,t2);
	  if (t1<tmin || t1>tmax)
	    t1=t2;
	  if (t2<tmin || t2>tmax)
	    t2=t1;
	  float3d t=usetmax?t2:t1;
	  if (t<=tmin || t>=tmax)
	    continue;
	  if (tmin_>t)
	    tmin_=t;
	  if (tmax_<t)
	    tmax_=t;	  
	}
      }
      vector<float3d> interpoly;
      for (int k=0;k<polyedre_abcv.size();++k){
	if (!polyedre_filled[k]) continue;
	float3d3 abc=polyedre_abcv[k];
	float3d a=abc.x,b=abc.y,c=abc.z;
	// intersect z=a*x+b*y+c with line m+t*v
	// m.z+t*v.z=a*(m.x+t*v.x)+b*(m.y+t*v.y)+c
	float3d A=v.z-a*v.x-b*v.y,B=a*m.x+b*m.y+c-m.z;
	if (A!=0){
	  float3d t=B/A;
	  float3d x=m.x+t*v.x;
	  float3d y=m.y+t*v.y;
	  if (t>tmin && t<tmax && inside(polyedrev[k],x,y)){
	    interpoly.push_back(t);
	  }
	}
      }
      if (interpoly.size()>=1){
	sort(interpoly.begin(),interpoly.end());
	float3d t=usetmax?interpoly.back():interpoly.front();
	if (tmin_>t)
	  tmin_=t;
	if (tmax_<t)
	  tmax_=t;
      }
      vecteur sv(gen2vecteur(g));
      for (int k=0;k<sv.size();++k){
	gen surf=remove_at_pnt(sv[k]);
	if (surf.is_symb_of_sommet(at_hypersurface)){
	  const vecteur & hyp=*surf._SYMBptr->feuille._VECTptr;
	  if (hyp.size()>2 && !is_undef(hyp[1])){
	    gen eq=hyp[1],vars=hyp[2];
	    if (_is_polynomial(makesequence(eq,vars[0]),contextptr)==1 && _is_polynomial(makesequence(eq,vars[1]),contextptr)==1 && _is_polynomial(makesequence(eq,vars[2]),contextptr)==1){
	      vecteur V(makevecteur(mx+vx*t__IDNT_e,my+vy*t__IDNT_e,mz+vz*t__IDNT_e));
	      gen eqt=subst(eq,vars,V,false,contextptr);
	      gen gradeq=subst(derive(eq,vars,contextptr),vars,V,false,contextptr); // not used
	      vecteur tval_=gen2vecteur(_sort(solve(eqt,t__IDNT_e,0,contextptr),contextptr));
	      int s=tval_.size();
	      vector<float3d> tval;
	      for (int l=0;l<s;++l){
		gen tg=evalf_double(tval_[l],1,contextptr);
		if (tg.type==_DOUBLE_){
		  float3d t=tg._DOUBLE_val;
		  if (t>=tmin && t<=tmax){
		    if (tval.size()>1)
		      tval.back()=t;
		    else
		      tval.push_back(t);
		  }
		}
	      }
#if 1
	      if (!tval.empty()){
		float3d t=usetmax?tval.back():tval.front();
		if (tmin_>t)
		  tmin_=t;
		if (tmax_<t)
		  tmax_=t;
	      }
#else
	      if (tval.size()==1){
		// grad of eq dot [vx,vy.vz] is positive if entering surface
		gen gradt=subst(gradeq,t__IDNT_e,tval[0],false,contextptr);
		gen dotp=dotvecteur(gen2vecteur(gradt),makevecteur(vx,vy,vz));
		if (is_positive(dotp,contextptr)){
		  if (tmin_>tval[0])
		    tmin_=tval[0];
		  tmax_=tmax;
		}
		else {
		  if (tmax_<tval[0])
		    tmax_=tval[0];
		  tmin_=tmin;
		}
	      }
	      else if (tval.size()==2){
		if (tmin_>tval[0])
		  tmin_=tval[0];
		if (tmax_<tval[1])
		  tmax_=tval[1];
	      }
#endif
	    } // end polynomial hypersurface
	  }
	} // end hypersurface	
      }
      int i1,j1,i2,j2;
      grmtv2ij(*this,m,v,tmin,i1,j1);
      grmtv2ij(*this,m,v,tmax,i2,j2);
      if (tmin==tmin_ && tmax==tmax_){
	int curcolor=color.u;
	if (0 && i1<jmintabsize){
	  if (jmaxtab[i1]>=jmintab[i1] && j1>=jmaxtab[i1] )
	    curcolor=color.d;
	}
	drawLine(i1,j1,i2,j2,curcolor);
      }
      else {
	int curcolor=color.d;
	if (0 && i1<jmintabsize){
	  if (jmaxtab[i1]>=jmintab[i1] && j1<jmintab[i1])
	    curcolor=color.u;
	}
	drawLine(i1,j1,i2,j2,curcolor | 0x400000);
      }
      bool name=show_names && lines[j];
      if (tmin<tmin_){
	grmtv2ij(*this,m,v,tmin,i1,j1);
	grmtv2ij(*this,m,v,tmin_,i2,j2);
	int curcolor=color.u;
	if (0 && i1<jmintabsize){
	  if (jmaxtab[i1]>=jmintab[i1] && j1>=jmaxtab[i1] )
	    curcolor=color.d;
	}
	drawLine(i1,j1,i2,j2,curcolor);
	if (name){
	  os_draw_string(i1,j1,color.u,0,lines[j]);
	  name=false;
	}
	if (tmin_!=tmax) os_fill_rect(i2-2,j2-2,4,4,curcolor);
      }
      if (tmax>tmax_){
	grmtv2ij(*this,m,v,tmax_,i1,j1);
	grmtv2ij(*this,m,v,tmax,i2,j2);
	int curcolor=color.u;
	if (0 && i1<jmintabsize){
	  if (jmaxtab[i1]>=jmintab[i1] && j1>=jmaxtab[i1] )
	    curcolor=color.d;
	}
	drawLine(i1,j1,i2,j2,curcolor);
	if (name){
	  os_draw_string(i2,j2,color.u,0,lines[j]);
	  name=false;
	}
	if (tmax_!=tmin) os_fill_rect(i1-2,j1-2,4,4,curcolor);
      }
    }
#endif // OLD_LINE_RENDERING
    //os_sync_screen();
    return true;
  }

  Graph2d * geoptr=0;

  // return true if there is a syntax error and user asked to correct
  bool geoparse(textArea *text,GIAC_CONTEXT){
#ifdef CASCAS_ALEVEL_ONLY
    return false;
#else
    Graph2d * geoptr=text->gr;
    if (!geoptr)
      return false;
    std::vector<textElement> & v=text->elements;
    geoptr->symbolic_instructions.resize(v.size());
    int pos=-1,i=0;
    for (;i<int(v.size());++i){
      std::string s=v[i].s; 
      giac::python_compat(0,contextptr);
      freeze=true;
      giac::gen g(s,contextptr);
      freeze=false;
      g=equaltosto(g,contextptr);
      int lineerr=giac::first_error_line(contextptr);
      char status[256]={0};
      geoptr->symbolic_instructions[i]=g;
      if (lineerr){
	std::string tok=giac::error_token_name(contextptr);
	if (lineerr==1){
	  pos=v[i].s.find(tok);
	  const std::string & err=v[i].s;
	  if (pos>=err.size())
	    pos=-1;
	}
	else {
	  tok=(lang==1)?"la fin":"end";
	  pos=0;
	}
	if (pos>=0)
	  sprintf(status,(lang==1)?"Erreur ligne %i a %s":"Error line %i at %s",i+1,tok.c_str());
	else
	  sprintf(status,(lang==1)?"Erreur ligne %i %s":"Error line %i %s",i+1,(pos==-2?((lang==1)?", : manquant ?":", missing :?"):""));
	if (confirm(status,(lang==1)?"OK: corrige, back: continue":"OK: fix",1)==KEY_CTRL_F1){
	  text->line=i;
	  if (pos>=0 && pos<v[i].s.size()) text->pos=pos;
	  return true;
	}
      }
    } // loop on lines
    return false;
#endif
  }

  int geoloop(Graph2d * geoptr){
#ifdef CASCAS_ALEVEL_ONLY
    return -1;
#else
    if (!geoptr || !geoptr->hp) return -1;
    const context * contextptr=geoptr->contextptr;
    textArea * text=geoptr->hp;
    // main loop: alternate between plot and symb view
    // start in plot view
    // end plot view with EXIT or OK -> symb view editor
    // end with OK or EXIT: OK will modify, EXIT will leave geo app
    // (press twice EXIT to leave geo app from plot view)
    for (;;){
      geoptr->eval();
      geoptr->update();
      if (geoptr->is3d)
	geoptr->update_rotation();
      int key=geoptr->ui();
      if (key==KEY_SHUTDOWN){
	geosave(text,contextptr);
	return key;
      }
      // symb view editor
      for (;;){
	key=doTextArea(text);
        //confirm("geo textarea out",print_INT_(key).c_str());
	if (key== TEXTAREA_RETURN_EXIT || key==KEY_SHUTDOWN){
	  geosave(text,contextptr);
	  return key;
	}
	// key was OK, parse step: synchronize symbolic_instructions from text
	bool corrige=geoparse(text,contextptr);
	if (!corrige)
	  break;
      } // end edition loop
    } // end plot/symb view infinite loop
#endif
  }

  void cleargeo(){
#ifdef CASCAS_ALEVEL_ONLY
    geoptr=0;
    return;
#else
    if (!geoptr)
      return;
    if (geoptr->hp)
      delete geoptr->hp;
    delete geoptr;
    geoptr=0;
#endif
  }

  int newgeo(GIAC_CONTEXT){
#ifdef CASCAS_ALEVEL_ONLY
    return -1;
#else
    if (!geoptr){
      geoptr=new Graph2d(0,contextptr);
      geoptr->window_xmin=-5;
      geoptr->window_ymin=-5;
      geoptr->window_zmin=-5;
      geoptr->window_xmax=5;
      geoptr->window_ymax=5;
      geoptr->window_zmax=5;
      geoptr->orthonormalize();
    }
    if (!geoptr)
      return -1;
    if (!geoptr->hp){
      geoptr->hp=new textArea;
      geoptr->hp->filename="figure0.py";
      geoptr->hp->python=0;
    }
    if (!geoptr->hp)
      return -2;
    textArea * text=geoptr->hp;
    text->editable=true;
    text->clipline=-1;
    text->gr=geoptr;
    geoptr->set_mode(0,0,255,""); // start in frame mode
    return 0;
#endif
  }

  Graph2d::Graph2d(const giac::gen & g_,const giac::context * cptr):window_xmin(gnuplot_xmin),window_xmax(gnuplot_xmax),window_ymin(gnuplot_ymin),window_ymax(gnuplot_ymax),window_zmin(gnuplot_zmin),window_zmax(gnuplot_zmax),g(g_),display_mode(0x45),show_axes(1),show_edges(1),show_names(1),labelsize(16),precision(2),contextptr(cptr),hp(0),npixels(5),couleur(0),nparams(0) {
    tracemode=0; tracemode_n=0; tracemode_i=0;
    current_i=width3d/3;
    current_j=LCD_HEIGHT_PX/3;
    push_depth=current_depth=0;
    diffusionz=5; diffusionz_limit=5; hide2nd=false; interval=false;
    default_upcolor=giac3d_default_upcolor;
    default_downcolor=giac3d_default_downcolor;
    default_downupcolor=giac3d_default_downupcolor;
    default_downdowncolor=giac3d_default_downdowncolor;
    doprecise=false;
    lcdz= LCD_HEIGHT_PX/4;
    is3d=giac::is3d(g);
    //theta_x=theta_y=theta_z=0;
    q=quaternion_double(0,0,0);//rotation_2_quaternion_double(0.707,0.707,0,1); 
    update_scales();
    autoscale(false,!is3d);
    update_rotation();
    if (is3d){
      if (surfacev.empty()){
	// no hypersurface inside, 2 for polyhedron
	precision=1;
      }
    }
  }

  void Graph2d::zoomx(double d,bool round,bool doupdate){
    double x_center=(window_xmin+window_xmax)/2;
    double dx=(window_xmax-window_xmin);
    if (dx==0)
      dx=gnuplot_xmax-gnuplot_xmin;
    dx *= d/2;
    x_tick = find_tick(dx);
    window_xmin = x_center - dx;
    if (round) 
      window_xmin=int( window_xmin/x_tick -1)*x_tick;
    window_xmax = x_center + dx;
    if (round)
      window_xmax=int( window_xmax/x_tick +1)*x_tick;
    if (doupdate)
      update();
  }

  void Graph2d::zoomy(double d,bool round,bool doupdate){
    double y_center=(window_ymin+window_ymax)/2;
    double dy=(window_ymax-window_ymin);
    if (dy==0)
      dy=gnuplot_ymax-gnuplot_ymin;
    dy *= d/2;
    y_tick = find_tick(dy);
    window_ymin = y_center - dy;
    if (round)
      window_ymin=int( window_ymin/y_tick -1)*y_tick;
    window_ymax = y_center + dy;
    if (round)
      window_ymax=int( window_ymax/y_tick +1)*y_tick;
    if (doupdate)
      update();
  }

  void Graph2d::zoomz(double d,bool round,bool doupdate){
    double z_center=(window_zmin+window_zmax)/2;
    double dz=(window_zmax-window_zmin);
    if (dz==0)
      dz=gnuplot_zmax-gnuplot_zmin;
    dz *= d/2;
    z_tick = find_tick(dz);
    window_zmin = z_center - dz;
    if (round) 
      window_zmin=int( window_zmin/z_tick -1)*z_tick;
    window_zmax = z_center + dz;
    if (round)
      window_zmax=int( window_zmax/z_tick +1)*z_tick;
    if (doupdate)
      update();
  }

  void Graph2d::zoom(double d,bool doupdate){ 
    zoomx(d,false,false);
    zoomy(d,false,false);
    zoomz(d,false,false);
    if (doupdate)
      update();
  }

  void Graph2d::autoscale(bool fullview,bool doupdate){
    // Find the largest and lowest x/y/z in objects (except lines/plans)
    vector<double> vx,vy,vz;
    int s;
    bool ortho=autoscaleg(g,vx,vy,vz,contextptr);
    autoscaleminmax(vx,window_xmin,window_xmax,fullview);
    double zf=is3d?1.03:1+1e-14;
    zoomx(zf,false,false);
    autoscaleminmax(vy,window_ymin,window_ymax,fullview);
    zoomy(zf,false,false);
    autoscaleminmax(vz,window_zmin,window_zmax,fullview);
    zoomz(zf,false,false);
    if (window_xmax-window_xmin<1e-100){
      window_xmax=gnuplot_xmax;
      window_xmin=gnuplot_xmin;
    }
    if (window_ymax-window_ymin<1e-100){
      window_ymax=gnuplot_ymax;
      window_ymin=gnuplot_ymin;
    }
    if (window_zmax-window_zmin<1e-100){
      window_zmax=gnuplot_zmax;
      window_zmin=gnuplot_zmin;
    }
    bool do_ortho=ortho;
    if (!do_ortho){
      double w=LCD_WIDTH_PX;
      double h=LCD_HEIGHT_PX-STATUS_AREA_PX;
      double window_w=window_xmax-window_xmin,window_h=window_ymax-window_ymin;
      double tst=h/w*window_w/window_h;
      if (tst>0.7 && tst<1.4)
	do_ortho=true;
    }
    if (do_ortho )
      orthonormalize(false);
    y_tick=find_tick(window_ymax-window_ymin);
    if (doupdate)
      update();
  }


  void Graph2d::orthonormalize(bool doupdate){
    // Center of the directions, orthonormalize
    double w=width3d;
    double h=LCD_HEIGHT_PX-STATUS_AREA_PX;
    double window_w=window_xmax-window_xmin,window_h=window_ymax-window_ymin;
    if (is3d){
      double window_xcenter=(window_xmin+window_xmax)/2;
      double window_ycenter=(window_ymin+window_ymax)/2;
      double window_zcenter=(window_zmin+window_zmax)/2;
      double window_z=window_zmax-window_zmin;
      double wmax=std::max(window_w,std::max(window_h,window_z));
      window_xmin=window_xcenter-wmax/2;
      window_xmax=window_xcenter+wmax/2;
      window_ymin=window_ycenter-wmax/2;
      window_ymax=window_ycenter+wmax/2;
      window_zmin=window_zcenter-wmax/2;
      window_zmax=window_zcenter+wmax/2;
      z_tick=find_tick(window_zmax-window_zmin);
    }
    else {
      w=LCD_WIDTH_PX;
      double window_hsize=h/w*window_w;
      if (window_h > window_hsize*1.01){ // enlarge horizontally
	double window_xcenter=(window_xmin+window_xmax)/2;
	double window_wsize=w/h*window_h;
	window_xmin=window_xcenter-window_wsize/2;
      window_xmax=window_xcenter+window_wsize/2;
      }
      if (window_h < window_hsize*0.99) { // enlarge vertically
	double window_ycenter=(window_ymin+window_ymax)/2;
	window_ymin=window_ycenter-window_hsize/2;
	window_ymax=window_ycenter+window_hsize/2;
      }
    }
    x_tick=find_tick(window_xmax-window_xmin);
    y_tick=find_tick(window_ymax-window_ymin);
    if (doupdate)
      update();
  }

  void Graph2d::update_scales(){
    if (is3d){
      x_scale=1.414*(LCD_HEIGHT_PX-STATUS_AREA_PX)/(window_xmax-window_xmin);    
      y_scale=1.414*(LCD_HEIGHT_PX-STATUS_AREA_PX)/(window_ymax-window_ymin);    
      z_scale=(LCD_HEIGHT_PX-STATUS_AREA_PX)/(window_zmax-window_zmin);    
    }
    else {
      x_scale=LCD_WIDTH_PX/(window_xmax-window_xmin);    
      y_scale=(LCD_HEIGHT_PX-STATUS_AREA_PX)/(window_ymax-window_ymin);    
    }
  }

  void Graph2d::update(){
    update_scales();
    if (is3d) update_rotation();
  }
  
  void mult4(float3d * c,float3d k,float3d * res){
    for (int i=0;i<16;i++)
      res[i]=k*c[i];
  }
  
  float3d det4(float3d * c){
    return c[0]*c[5]*c[10]*c[15]-c[0]*c[5]*c[14]*c[11]-c[0]*c[9]*c[6]*c[15]+c[0]*c[9]*c[14]*c[7]+c[0]*c[13]*c[6]*c[11]-c[0]*c[13]*c[10]*c[7]-c[4]*c[1]*c[10]*c[15]+c[4]*c[1]*c[14]*c[11]+c[4]*c[9]*c[2]*c[15]-c[4]*c[9]*c[14]*c[3]-c[4]*c[13]*c[2]*c[11]+c[4]*c[13]*c[10]*c[3]+c[8]*c[1]*c[6]*c[15]-c[8]*c[1]*c[14]*c[7]-c[8]*c[5]*c[2]*c[15]+c[8]*c[5]*c[14]*c[3]+c[8]*c[13]*c[2]*c[7]-c[8]*c[13]*c[6]*c[3]-c[12]*c[1]*c[6]*c[11]+c[12]*c[1]*c[10]*c[7]+c[12]*c[5]*c[2]*c[11]-c[12]*c[5]*c[10]*c[3]-c[12]*c[9]*c[2]*c[7]+c[12]*c[9]*c[6]*c[3];
  }

  void inv4(float3d * c,float3d * res){
    res[0]=c[5]*c[10]*c[15]-c[5]*c[14]*c[11]-c[10]*c[7]*c[13]-c[15]*c[9]*c[6]+c[14]*c[9]*c[7]+c[11]*c[6]*c[13];
    res[1]=-c[1]*c[10]*c[15]+c[1]*c[14]*c[11]+c[10]*c[3]*c[13]+c[15]*c[9]*c[2]-c[14]*c[9]*c[3]-c[11]*c[2]*c[13];
    res[2]=c[1]*c[6]*c[15]-c[1]*c[14]*c[7]-c[6]*c[3]*c[13]-c[15]*c[5]*c[2]+c[14]*c[5]*c[3]+c[7]*c[2]*c[13];
    res[3]=-c[1]*c[6]*c[11]+c[1]*c[10]*c[7]+c[6]*c[3]*c[9]+c[11]*c[5]*c[2]-c[10]*c[5]*c[3]-c[7]*c[2]*c[9];
    res[4]=-c[4]*c[10]*c[15]+c[4]*c[14]*c[11]+c[10]*c[7]*c[12]+c[15]*c[8]*c[6]-c[14]*c[8]*c[7]-c[11]*c[6]*c[12];
    res[5]=c[0]*c[10]*c[15]-c[0]*c[14]*c[11]-c[10]*c[3]*c[12]-c[15]*c[8]*c[2]+c[14]*c[8]*c[3]+c[11]*c[2]*c[12];
    res[6]=-c[0]*c[6]*c[15]+c[0]*c[14]*c[7]+c[6]*c[3]*c[12]+c[15]*c[4]*c[2]-c[14]*c[4]*c[3]-c[7]*c[2]*c[12];
    res[7]=c[0]*c[6]*c[11]-c[0]*c[10]*c[7]-c[6]*c[3]*c[8]-c[11]*c[4]*c[2]+c[10]*c[4]*c[3]+c[7]*c[2]*c[8];
    res[8]=c[4]*c[9]*c[15]-c[4]*c[13]*c[11]-c[9]*c[7]*c[12]-c[15]*c[8]*c[5]+c[13]*c[8]*c[7]+c[11]*c[5]*c[12];
    res[9]=-c[0]*c[9]*c[15]+c[0]*c[13]*c[11]+c[9]*c[3]*c[12]+c[15]*c[8]*c[1]-c[13]*c[8]*c[3]-c[11]*c[1]*c[12];
    res[10]=c[0]*c[5]*c[15]-c[0]*c[13]*c[7]-c[5]*c[3]*c[12]-c[15]*c[4]*c[1]+c[13]*c[4]*c[3]+c[7]*c[1]*c[12];
    res[11]=-c[0]*c[5]*c[11]+c[0]*c[9]*c[7]+c[5]*c[3]*c[8]+c[11]*c[4]*c[1]-c[9]*c[4]*c[3]-c[7]*c[1]*c[8];
    res[12]=-c[4]*c[9]*c[14]+c[4]*c[13]*c[10]+c[9]*c[6]*c[12]+c[14]*c[8]*c[5]-c[13]*c[8]*c[6]-c[10]*c[5]*c[12];
    res[13]=c[0]*c[9]*c[14]-c[0]*c[13]*c[10]-c[9]*c[2]*c[12]-c[14]*c[8]*c[1]+c[13]*c[8]*c[2]+c[10]*c[1]*c[12];
    res[14]=-c[0]*c[5]*c[14]+c[0]*c[13]*c[6]+c[5]*c[2]*c[12]+c[14]*c[4]*c[1]-c[13]*c[4]*c[2]-c[6]*c[1]*c[12];
    res[15]=c[0]*c[5]*c[10]-c[0]*c[9]*c[6]-c[5]*c[2]*c[8]-c[10]*c[4]*c[1]+c[9]*c[4]*c[2]+c[6]*c[1]*c[8];
    float3d det=det4(c);
    mult4(res,1/det,res);
  }

  void Graph2d::xyz2ij(const float3d3 &d,int &i,int &j) const {
    float3d X,Y,Z;
    do_transform(transform,d.x,d.y,d.z,X,Y,Z);
    i=width3d/2+(Y-X)/4.8*width3d;
    j=LCD_HEIGHT_PX/2-Z*lcdz+(Y+X)/9.6*width3d;    
  }
  
  void Graph2d::xyz2ij(const float3d3 &d,float3d &i,float3d &j) const {
    float3d X,Y,Z;
    do_transform(transform,d.x,d.y,d.z,X,Y,Z);
    i=width3d/2+(Y-X)/4.8*width3d;
    j=LCD_HEIGHT_PX/2-Z*lcdz+(Y+X)/9.6*width3d;    
  }
  
  void Graph2d::xyz2ij(const float3d3 &d,float3d &i,float3d &j,float3d3 & d3) const {
    do_transform(transform,d.x,d.y,d.z,d3.x,d3.y,d3.z);
    i=width3d/2+(d3.y-d3.x)/4.8*width3d;
    j=LCD_HEIGHT_PX/2-d3.z*lcdz+(d3.y+d3.x)/9.6*width3d;    
  }
  
  void Graph2d::XYZ2ij(const float3d3 &d,int &i,int &j) const {
    float3d X=d.x,Y=d.y,Z=d.z;
    i=width3d/2+(Y-X)/4.8*width3d;
    j=LCD_HEIGHT_PX/2-Z*lcdz+(Y+X)/9.6*width3d;    
  }

  void Graph2d::update_rotation(){
    solid3d=false;
    double rx,ry,rz,theta;
    get_axis_angle_deg(q,rx,ry,rz,theta);
    // rx=-0.51; ry=-.197; rz=-.835; theta=327.88;
    // rx=0;ry=1;rz=0; //theta=60;
    // cout << rx << " " << ry << " " << rz << " " << theta << "\n";
    // rx=0;ry=0;rz=1;
    // theta=10;
    theta *= M_PI/180;
    float3d r2=rx*rx+ry*ry+rz*rz,invr2=1/r2;
    float3d r=std::sqrt(r2);
    float3d c=std::cos(theta),s=std::sin(theta);
    // mkisom([[rx,ry,rz],theta],1)
    // 1/r2*[[rx*rx+ry*ry*c+rz*rz*c,rx*ry-rx*ry*c-rz*s*r,rx*rz-rx*rz*c+ry*s*r],[rx*ry-rx*ry*c+rz*s*r,ry*ry+rx*rx*c+rz*rz*c,ry*rz-rx*s*r-ry*rz*c],[rx*rz-rx*rz*c-ry*s*r,ry*rz+rx*s*r-ry*rz*c,rz*rz+rx*rx*c+ry*ry*c]]
    float3d a11=invr2*(rx*rx+ry*ry*c+rz*rz*c),a12=invr2*(rx*ry-rx*ry*c-rz*s*r),a13=invr2*(rx*rz-rx*rz*c+ry*s*r);
    float3d a21=invr2*(rx*ry-rx*ry*c+rz*s*r),a22=invr2*(ry*ry+rx*rx*c+rz*rz*c),a23=invr2*(ry*rz-rx*s*r-ry*rz*c);
    float3d a31=invr2*(rx*rz-rx*rz*c-ry*s*r),a32=invr2*(ry*rz+rx*s*r-ry*rz*c),a33=invr2*(rz*rz+rx*rx*c+ry*ry*c);
    float3d xt=(window_xmin+window_xmax)/2,xs=2.0/(window_xmax-window_xmin),yt=(window_ymin+window_ymax)/2,ys=2.0/(window_ymax-window_ymin),zt=(window_zmin+window_zmax)/2,zs=2.0/(window_zmax-window_zmin);
    float3d mat[16]={a11*xs,a12*ys,a13*zs,-a11*xt*xs-a12*yt*ys-a13*zt*zs,
		    a21*xs,a22*ys,a23*zs,-a21*xt*xs-a22*yt*ys-a23*zt*zs,
		    a31*xs,a32*ys,a33*zs,-a31*xt*xs-a32*yt*ys-a33*zt*zs,
		    0,0,0,1
    };
    for (int i=0;i<sizeof(mat)/sizeof(float3d);++i)
      transform[i]=mat[i];
    inv4(transform,invtransform);
    // for (int i=0;i<16;++i){ cout << transform[i] << (i%4==3?"\n":" "); } cout << "\n";
    // for (int i=0;i<16;++i){ cout << invtransform[i] << (i%4==3?"\n":" ");} cout << "\n";
    surfacev.clear();
    polyedrev.clear(); polyedre_xyminmax.clear(); polyedre_abcv.clear();polyedre_faceisclipped.clear(); polyedre_filled.clear();
    plan_pointv.clear(); plan_abcv.clear(); plan_filled.clear();
    sphere_centerv.clear(); sphere_radiusv.clear(); sphere_quadraticv.clear();
    linev.clear(); linetypev.clear(); curvev.clear();
    pointv.clear(); points.clear();
    plan_color.clear();sphere_color.clear();polyedre_color.clear();line_color.clear();curve_color.clear(); hyp_color.clear(); point_color.clear();
    // rotate+translate+scale g
    vecteur v;
    aplatir(gen2vecteur(g),v);
    //cout << "vs" << int(v.size()) << "\n";
    for (int i=0;i<v.size();++i){
      int u=default_upcolor,d=default_downcolor,du=default_downupcolor,dd=default_downdowncolor;
      const char * ptr=0;
      bool fill_polyedre=false;
      if (v[i].is_symb_of_sommet(at_pnt)){
	vecteur & attrv=*v[i]._SYMBptr->feuille._VECTptr;
	//cout << "vi " << i << " " << int(attrv.size()) << "\n";
	if (attrv.size()>1){
	  gen attr=attrv[1];
	  fill_polyedre=get_colors(attr,u,d,du,dd);
	  if (fill_polyedre) solid3d=true;
	  if (attrv.size()>2){
	    attr=attrv[2];
	    if (attr.type==_STRNG)
	      ptr=attr._STRNGptr->c_str();
	    if (attr.type==_IDNT)
	      ptr=attr._IDNTptr->id_name;
	  }
	}
      }
      gen G=remove_at_pnt(v[i]);
      if (G.is_symb_of_sommet(at_curve)){
	// cout << "curve" << '\n';
	gen f=G._SYMBptr->feuille;
	f=f[0];
	f=f[4];
	if (ckmatrix(f)){
	  vecteur v=*f._VECTptr;
	  int n=v.size();
	  curvev.push_back(vector<float3d3>(n));
	  curve_color.push_back(int4(u,d,du,dd));
	  vector<float3d3> & cur=curvev.back();
	  for (int k=0;k<n;++k){
	    gen P=v[k];
	    if (P.type==_VECT && P._VECTptr->size()==3){
	      float3d X,Y,Z;
	      do_transform(transform,P[0]._DOUBLE_val,P[1]._DOUBLE_val,P[2]._DOUBLE_val,X,Y,Z);
	      cur[k]=float3d3(X,Y,Z);
	    }
	  }
	}
	continue;
      }
      if (G.type==_VECT && G.subtype==_POINT__VECT){
	gen m=evalf_double(G,1,contextptr);
	if (m.type==_VECT && m._VECTptr->size()==3){
	  vecteur & A=*m._VECTptr;
	  if (A[0].type==_DOUBLE_ && A[1].type==_DOUBLE_ && A[2].type==_DOUBLE_ ){
	    float3d X,Y,Z; int I,J;
	    do_transform(transform,A[0]._DOUBLE_val,A[1]._DOUBLE_val,A[2]._DOUBLE_val,X,Y,Z);
	    xyz2ij(float3d3(A[0]._DOUBLE_val,A[1]._DOUBLE_val,A[2]._DOUBLE_val),I,J);
	    pointv.push_back(float3d3(I,J,Z));
	    points.push_back(ptr);
	    point_color.push_back(int4(u,d,du,dd));
	  }
	}
	continue;
      }
      bool line=G.subtype==_LINE__VECT,halfline=G.subtype==_HALFLINE__VECT,segment= G.subtype==_GROUP__VECT;
      if (G.type==_VECT && G._VECTptr->size()>=2 && (line || halfline || segment)){
	for (int n=1;n<G._VECTptr->size();++n){
	  gen a=evalf_double((*G._VECTptr)[n-1],1,contextptr),b=evalf_double((*G._VECTptr)[n],1,contextptr);
	  if (a.type==_VECT && b.type==_VECT && a._VECTptr->size()==3 && b._VECTptr->size()==3){
	    vecteur & A=*a._VECTptr;
	    vecteur & B=*b._VECTptr;
	    if (A[0].type==_DOUBLE_ && A[1].type==_DOUBLE_ && A[2].type==_DOUBLE_ && B[0].type==_DOUBLE_ && B[1].type==_DOUBLE_ && B[2].type==_DOUBLE_ ){
	      lines.push_back(ptr);
	      float3d x=A[0]._DOUBLE_val,y=A[1]._DOUBLE_val,z=A[2]._DOUBLE_val;
#if 0 // ndef OLD_LINE_RENDERING
	      float3d3 prev(x,y,z);
	      linev.push_back(prev);
#endif
	      float3d X,Y,Z;
	      do_transform(transform,x,y,z,X,Y,Z);
	      float3d3 M(X,Y,Z);
	      x=B[0]._DOUBLE_val;y=B[1]._DOUBLE_val;z=B[2]._DOUBLE_val;
#if 0 // ndef OLD_LINE_RENDERING
	      linev.push_back(float3d3(x-prev.x,y-prev.y,z-prev.z));
#endif
	      do_transform(transform,x,y,z,X,Y,Z);
	      float3d3 N(X,Y,Z);
	      float3d3 v(N.x-M.x,N.y-M.y,N.z-M.z);
	      linev.push_back(M); linev.push_back(v);
	      linetypev.push_back(G.subtype);
	      line_color.push_back(int4(u,d,du,dd));
	    }
	  }
	}
	continue;
      }
      if (G.is_symb_of_sommet(at_hypersphere)){
	solid3d=true;
	vecteur hyp=*G._SYMBptr->feuille._VECTptr;
	gen c=evalf_double(hyp[0],1,contextptr);
	float3d x=c[0]._DOUBLE_val,y=c[1]._DOUBLE_val,z=c[2]._DOUBLE_val,X,Y,Z;
	do_transform(transform,x,y,z,X,Y,Z);
	sphere_centerv.push_back(float3d3(X,Y,Z));
	gen R=evalf(hyp[1],1,contextptr);
	float3d r=R._DOUBLE_val;
	sphere_radiusv.push_back(r);
	float3d * mat=invtransform;
	matrice qmat(makevecteur(
				 makevecteur(mat[0],mat[4],mat[8]),
				 makevecteur(mat[1],mat[5],mat[9]),
				 makevecteur(mat[2],mat[6],mat[10])
				 ));
	qmat=mmult(mtran(qmat),qmat);
	sphere_quadraticv.push_back(qmat);
	sphere_color.push_back(int4(u,d,du,dd));
	bool isclipped=x>=window_xmin+r && x<=window_xmax-r && y>=window_ymin+r && y<=window_ymax-r && z>=window_zmin+r && z<=window_zmax-r;
	// check if distance of center to window_x/y/xmin/max is <=R
	sphere_isclipped.push_back(isclipped);
	continue;
      }
      if (G.is_symb_of_sommet(at_hyperplan)){
	plan_filled.push_back(fill_polyedre);
	vecteur hyp=*G._SYMBptr->feuille._VECTptr;
	gen hyp1=evalf_double(hyp[1],1,contextptr);
	vecteur & hyp1v=*hyp1._VECTptr;
	float3d A,B,C,x,y,z,X,Y,Z;
	A=hyp1v[0]._DOUBLE_val;B=hyp1v[1]._DOUBLE_val;C=hyp1v[2]._DOUBLE_val;
	do_transform(transform,A,B,C,X,Y,Z);
	gen hyp0=evalf_double(hyp[0],1,contextptr);
	vecteur & hyp0v=*hyp0._VECTptr;
	x=hyp0v[0]._DOUBLE_val;y=hyp0v[1]._DOUBLE_val;z=hyp0v[2]._DOUBLE_val;
	float3d * mat=invtransform;
	A=mat[0]*x+mat[4]*y+mat[8]*z;
	B=mat[1]*x+mat[5]*y+mat[9]*z;
	C=mat[2]*x+mat[6]*y+mat[10]*z;
	float3d AB=fabs(A)+fabs(B);
	if (fabs(C)<1e-2*AB){
	  // almost vertical plane, equation A*(x-X)+B*(y-Y)=0
	  continue;
	}
	A/=C; B/=C;
	plan_pointv.push_back(float3d3(X,Y,Z));	
	plan_abcv.push_back(float3d3(-A,-B,Z+A*X+B*Y));
	plan_color.push_back(int4(u,d,du,dd));
	continue;
      }
      if (G.is_symb_of_sommet(at_hypersurface)){
	solid3d=true;
	const vecteur & hyp=*G._SYMBptr->feuille._VECTptr;
	gen hyp0=hyp[0];
	const vecteur & hyp0v=*hyp0._VECTptr;
	gen h=hyp0v[4];
	if (h.type==_VECT && h.subtype==_POLYEDRE__VECT)
	  G=h;
	else if (ckmatrix(h,true)){
	  // cout << "surf" << '\n';
	  bool cplx=has_i(h); // 4d hypersurface, encode color in a float+int
	  float3d argcplx;
	  surfacev.push_back(vector< vector<float3d> >(0));
	  vector< vector<float3d> > & S=surfacev.back();
	  const vecteur & V=*h._VECTptr;
	  S.reserve(V.size());
	  for (int j=0;j<V.size();++j){
	    gen Vj=V[j];
	    const vecteur & vj=*Vj._VECTptr;
	    S.push_back(vector<float3d>(0));
	    vector<float3d> &S_=S.back();
	    S_.reserve(vj.size());
	    for (int k=0;k<vj.size();k+=3){
	      float3d X,Y,Z;
	      if (cplx)
		do_transform(mat,vj[k]._DOUBLE_val,vj[k+1]._DOUBLE_val,absarg(vj[k+2],argcplx),X,Y,Z);
	      else
		do_transform(mat,vj[k]._DOUBLE_val,vj[k+1]._DOUBLE_val,vj[k+2]._DOUBLE_val,X,Y,Z);		
	      // vj[k]=X; vj[k+1]=Y; vj[k+2]=Z;
	      S_.push_back(X); S_.push_back(Y); 
	      if (cplx){
		float2 * fptr=(float2 *) &Z;
		fptr->f = Z;
		fptr->a = argcplx;
		S_.push_back(Z);
	      }
	      else
		S_.push_back(Z);
	    }
	  }
	  hyp_color.push_back(cplx?int4(-1,-1,-1,-1):int4(u,d,du,dd));
	  continue;
	  vector< vector<float3d> > & Sb=surfacev.back();
	  for (int i=0;i<Sb.size();++i){
	    for (int j=0;j<Sb[i].size();++j){
	      cout << Sb[i][j] << " " ;
	    }
	    cout << "\n";
	  }
	} // end quad hypersurface
      } // end hypersurface
      if (G.type==_VECT && G.subtype==_GROUP__VECT){
	G=gen(makevecteur(G),_POLYEDRE__VECT);
      }
      if (G.type==_VECT && G.subtype==_POLYEDRE__VECT){
	vecteur p=*G._VECTptr;
	polyedrev.reserve(polyedrev.size()+p.size());
	polyedre_color.reserve(polyedre_color.size()+p.size());
	polyedre_xyminmax.reserve(polyedre_xyminmax.size()+2*p.size());
	for (int j=0;j<p.size();++j){
	  bool is_clipped=true;
	  gen g=p[j];
	  if (g.type==_VECT){
	    vector<float3d3> cur;
	    vecteur w=*g._VECTptr;
	    cur.reserve(w.size()+1);
	    for (int k=0;k<w.size();++k){
	      gen P=evalf_double(w[k],1,contextptr);
	      if (P.type==_VECT && P._VECTptr->size()==3){
		float3d x=P[0]._DOUBLE_val,y=P[1]._DOUBLE_val,z=P[2]._DOUBLE_val;
		if (is_clipped && (x<window_xmin || x>window_xmax || y<window_ymin || y>window_ymax || z<window_zmin || z>window_zmax) )
		  is_clipped=false;
		float3d X,Y,Z;
		do_transform(transform,x,y,z,X,Y,Z);
		cur.push_back(float3d3(X,Y,Z));
	      }
	    }
	    if (cur.size()>=3){
	      float3d Z3;int l=0;
	      for (;l<cur.size()-2;++l){
		float3d x0=cur[l].x,y0=cur[l].y,z0=cur[l].z;
		float3d x1=cur[l+1].x,y1=cur[l+1].y,z1=cur[l+1].z;
		float3d x2=cur[l+2].x,y2=cur[l+2].y,z2=cur[l+2].z;
		float3d X1=x1-x0,Y1=y1-y0,Z1=z1-z0;
		float3d X2=x2-x0,Y2=y2-y0,Z2=z2-z0;
		float3d X3=Y1*Z2-Y2*Z1,Y3=X2*Z1-X1*Z2;Z3=X1*Y2-X2*Y1;
		float3d prec=1e-3;
#if 1
		if ( (X3!=0 || Y3!=0) && fabs(Z3)<prec*(fabs(X3)+fabs(Y3))){
		  Z3=1.0001*prec*(fabs(X3)+fabs(Y3));
		}
#endif
		if (fabs(Z3)>prec*(fabs(X3)+fabs(Y3))){
		  // X3*(x-x0)+Y3*(y-y0)+Z3*(z-z0)=0
		  X3/=Z3; Y3/=Z3;
		  polyedre_abcv.push_back(float3d3(-X3,-Y3,z0+X3*x0+Y3*y0));
		  break;
		}
	      }
	      if (l==cur.size()-2)
		continue;
	      cur.push_back(cur.front());
	      float3d facemin=1e306,facemax=-1e306;
	      if (fill_polyedre){
		for (int l=1;l<cur.size();++l){
		  float3d xy=cur[l].y-cur[l].x;
		  if (xy<facemin)
		    facemin=xy;
		  if (xy>facemax)
		    facemax=xy;
		  // replace unused z coordinate by slope
		  // cur[l].z=(cur[l].y-cur[l-1].y)/(cur[l].x-cur[l-1].x);
		}
	      }
	      polyedrev.push_back(vector<float3d3>(0)); polyedrev.back().swap(cur); // polyedrev.push_back(cur);
	      polyedre_color.push_back(int4(u,d,du,dd));
	      polyedre_xyminmax.push_back(facemin);
	      polyedre_xyminmax.push_back(facemax);
	      polyedre_faceisclipped.push_back(is_clipped);
	      polyedre_filled.push_back(fill_polyedre);
	    } // end cur.size()>=3
	  } // end g.type==_VECT
	}
	continue;
      }      
    }
  }

  bool Graph2d::findij(const gen & e0,double x_scale,double y_scale,double & i0,double & j0,GIAC_CONTEXT) const {
    gen e,f0,f1;
    evalfdouble2reim(e0,e,f0,f1,contextptr);
    if ((f0.type==_DOUBLE_) && (f1.type==_DOUBLE_)){
      if (display_mode & 0x400){
	if (f0._DOUBLE_val<=0)
	  return false;
	f0=std::log10(f0._DOUBLE_val);
      }
      i0=(f0._DOUBLE_val-window_xmin)*x_scale;
      if (display_mode & 0x800){
	if (f1._DOUBLE_val<=0)
	  return false;
	f1=std::log10(f1._DOUBLE_val);
      }
      j0=(window_ymax-f1._DOUBLE_val)*y_scale;
      return true;
    }
    // cerr << "Invalid drawing data" << endl;
    return false;
  }

  inline void swapint(int & i0,int & i1){
    int tmp=i0;
    i0=i1;
    i1=tmp;
  }

  void check_fl_draw(int fontsize,const char * ch,int i0,int j0,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    /* int n=fl_size();
       if (j0>=jmin-n && j0<=jmin+dj+n) */
    // cerr << i0 << " " << j0 << endl;
    if (strlen(ch)>200)
      text_print(fontsize,"String too long",i0+delta_i,j0+delta_j,c);
    else
      text_print(fontsize,ch,i0+delta_i,j0+delta_j,c);
  }

  inline void check_fl_point(int i0,int j0,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    /* if (i0>=imin && i0<=imin+di && j0>=jmin && j0<=jmin+dj) */
    set_pixel(i0+delta_i,j0+delta_j,c);
  }

  static unsigned short int fl_line_width=1;
  void fl_line(int x0,int y0,int x1,int y1,int c){
    if (fl_line_width==0)
      return;
    if (fl_line_width==1){
      draw_line(x0,y0,x1,y1,c);
      return;
    }
    double dx=x1-x0,dy=y1-y0;
    double n=std::sqrt(dx*dx+dy*dy);
    dx/=n; dy/=n;
#if 1
    if (dx==0){
      if (y0>y1) swapint(y0,y1);
      int h=y1-y0;
      drawRectangle(x0-fl_line_width/2,y0,fl_line_width,h,c);
    } else if (dy==0){
      if (x0>x1) swapint(x0,x1);
      int w=x1-x0;
      drawRectangle(x0,y0-fl_line_width/2,w,fl_line_width,c);      
    } else {
      vector< vector<int> > v;
      vector<int> w(2);
      w[0]=int(x0-fl_line_width*dy/2+.5);
      w[1]=int(y0+fl_line_width*dx/2+.5);
      v.push_back(w);
      w[0]=int(x0+fl_line_width*dy/2+.5);
      w[1]=int(y0-fl_line_width*dx/2+.5);
      v.push_back(w);
      w[0]=int(x1+fl_line_width*dy/2+.5);
      w[1]=int(y1-fl_line_width*dx/2+.5);
      v.push_back(w);
      w[0]=int(x1-fl_line_width*dy/2+.5);
      w[1]=int(y1+fl_line_width*dx/2+.5);
      v.push_back(w);
      draw_filled_polygon(v,0,LCD_WIDTH_PX,0,LCD_HEIGHT_PX,c);
    }
#else
    for (int d=-fl_line_width/2;d<=(fl_line_width+1)/2;++d){
      draw_line(x0-d*dy,y0+d*dx,x1-d*dy,y1+d*dx,c);
    }
#endif
    draw_filled_circle(x0,y0,(fl_line_width-1)/2,c,true,true);
    draw_filled_circle(x1,y1,(fl_line_width-1)/2,c,true,true);
  }

  inline void fl_polygon(int x0,int y0,int x1,int y1,int x2,int y2,int c){
    fl_line(x0,y0,x1,y1,c);
    fl_line(x1,y1,x2,y2,c);
    fl_line(x2,y2,x0,y0,c);
  }

  inline void check_fl_line(int i0,int j0,int i1,int j1,int imin,int jmin,int di,int dj,int delta_i,int delta_j,int c){
    fl_line(i0+delta_i,j0+delta_j,i1+delta_i,j1+delta_j,c);
  }

  int logplot_points=20;

  void checklog_fl_line(double i0,double j0,double i1,double j1,double deltax,double deltay,bool logx,bool logy,double window_xmin,double x_scale,double window_ymax,double y_scale,int c){
    if (!logx && !logy){
      fl_line(round(i0+deltax),round(j0+deltay),round(i1+deltax),round(j1+deltay),c);
      return;
    }
  }

  void find_dxdy(const string & legendes,int labelpos,int labelsize,int & dx,int & dy){
    int l=text_width(labelsize,legendes.c_str());
    dx=3;
    dy=1;
    switch (labelpos){
    case 1:
      dx=-l-3;
      break;
    case 2:
      dx=-l-3;
      dy=labelsize-2;
      break;
    case 3:
      dy=labelsize-2;
      break;
    }
    dy += labelsize;
  }

  void draw_legende(const vecteur & f,int i0,int j0,int labelpos,const Graph2d * iptr,int clip_x,int clip_y,int clip_w,int clip_h,int deltax,int deltay,int c){
    if (f.empty() ||!iptr->show_names )
      return;
    string legendes;
    if (f[0].is_symb_of_sommet(at_curve)){
      gen & f0=f[0]._SYMBptr->feuille;
      if (f0.type==_VECT && !f0._VECTptr->empty()){
	gen & f1 = f0._VECTptr->front();
	if (f1.type==_VECT && f1._VECTptr->size()>4 && (!is_zero((*f1._VECTptr)[4]) || (iptr->show_names & 2)) ){
	  gen legende=f1._VECTptr->front();
	  gen var=(*f1._VECTptr)[1];
	  gen r=re(legende,contextptr),i=im(legende,contextptr),a,b;
	  if (var.type==_IDNT && is_linear_wrt(r,*var._IDNTptr,a,b,contextptr)){
	    i=subst(i,var,(var-b)/a,false,contextptr);
	    legendes=i.print(contextptr);
	  }
	  else
	    legendes=r.print(contextptr)+","+i.print(contextptr);
	  if (legendes.size()>18){
	    if (legendes.size()>30)
	      legendes="";
	    else
	      legendes=legendes.substr(0,16)+"...";
	  }
	}
      }
    }
    if (f.size()>2)
      legendes=gen2string(f[2])+(legendes.empty()?"":":")+legendes;
    if (legendes.empty())
      return;
    int fontsize=iptr->labelsize;
    int dx=3,dy=1;
    find_dxdy(legendes,labelpos,fontsize,dx,dy);
    check_fl_draw(fontsize,legendes.c_str(),i0+dx,j0+dy,clip_x,clip_y,clip_w,clip_h,deltax,deltay,c);
  }

  void petite_fleche(double i1,double j1,double dx,double dy,int deltax,int deltay,int width,int c){
    double dxy=std::sqrt(dx*dx+dy*dy);
    if (dxy){
      dxy/=max(2,min(5,int(dxy/10)))+width;
      dx/=dxy;
      dy/=dxy;
      double dxp=-dy,dyp=dx; // perpendicular
      dx*=std::sqrt(3.0);
      dy*=std::sqrt(3.0);
      fl_polygon(round(i1)+deltax,round(j1)+deltay,round(i1+dx+dxp)+deltax,round(j1+dy+dyp)+deltay,round(i1+dx-dxp)+deltax,round(j1+dy-dyp)+deltay,c);
    }
  }

  void fltk_point(int deltax,int deltay,int i0,int j0,int epaisseur_point,int type_point,int c){
    switch (type_point){
    case 1: // losange
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0,deltay+j0-epaisseur_point,c);
      fl_line(deltax+i0,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0,deltay+j0+epaisseur_point,deltax+i0+epaisseur_point,deltay+j0,c);
      break;
    case 2: // croix verticale
      fl_line(deltax+i0,deltay+j0-epaisseur_point,deltax+i0,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0+epaisseur_point,deltay+j0,c);
      break;
    case 3: // carre
      fl_line(deltax+i0-epaisseur_point,deltay+j0-epaisseur_point,deltax+i0-epaisseur_point,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0+epaisseur_point,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0-epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0+epaisseur_point,deltax+i0+epaisseur_point,deltay+j0+epaisseur_point,c);
      break;
    case 5: // triangle
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0,deltay+j0-epaisseur_point,c);
      fl_line(deltax+i0,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0+epaisseur_point,deltay+j0,c);
      break;
    case 7: // point
      if (epaisseur_point>2)
	fl_arc(deltax+i0-(epaisseur_point-1),deltay+j0-(epaisseur_point-1),2*(epaisseur_point-1),2*(epaisseur_point-1),0,360,c);
      else
	fl_line(deltax+i0,deltay+j0,deltax+i0+1,deltay+j0,c);
      break;
    case 6: // etoile
      fl_line(deltax+i0-epaisseur_point,deltay+j0,deltax+i0+epaisseur_point,deltay+j0,c);
      // no break to add the following lines
    case 0: // 0 croix diagonale
      fl_line(deltax+i0-epaisseur_point,deltay+j0-epaisseur_point,deltax+i0+epaisseur_point,deltay+j0+epaisseur_point,c);
      fl_line(deltax+i0-epaisseur_point,deltay+j0+epaisseur_point,deltax+i0+epaisseur_point,deltay+j0-epaisseur_point,c);
      break;
    default: // 4 nothing drawn
      break;
    }
  }

  int horiz_or_vert(const_iterateur jt,GIAC_CONTEXT){
    gen tmp(*(jt+1)-*jt),r,i;
    reim(tmp,r,i,contextptr);
    if (is_zero(r,contextptr)) return 1;
    if (is_zero(i,contextptr)) return 2;
    return 0;
  }

  void fltk_draw(Graph2d & Mon_image,const gen & g,double x_scale,double y_scale,int clip_x,int clip_y,int clip_w,int clip_h){
    int deltax=0,deltay=STATUS_AREA_PX,fontsize=Mon_image.labelsize;
    if (g.type==_VECT){
      const vecteur & v=*g._VECTptr;
      const_iterateur it=v.begin(),itend=v.end();
      for (;it!=itend;++it)
	fltk_draw(Mon_image,*it,x_scale,y_scale,clip_x,clip_y,clip_w,clip_h);
    }
    if (g.type!=_SYMB)
      return;
    unary_function_ptr s=g._SYMBptr->sommet;
    if (g._SYMBptr->feuille.type!=_VECT)
      return;
    vecteur f=*g._SYMBptr->feuille._VECTptr;
    int mxw=LCD_WIDTH_PX,myw=LCD_HEIGHT_PX-STATUS_AREA_PX;
    double i0,j0,i0save,j0save,i1,j1;
    int fs=f.size();
    if (s==at_parameter && fs>=4 && f[0].type==_IDNT){
      // display parameter from the left upper, f[0] name and f[3] value
      char ch[128];
      strcpy(ch,f[0]._IDNTptr->id_name);
      int pos=strlen(ch);
      ch[pos]='=';
      ++pos;
      ch[pos]=0;
      gen g=evalf_double(f[3],1,contextptr);
      if (g.type==_DOUBLE_)
	strcpy(ch+pos,g.print(contextptr).c_str());
      else {
	ch[pos]='?';
	++pos;
	ch[pos]=0;
      }
      ++Mon_image.nparams;
      int dw=fl_width(ch);
      int fheight=14;
      int ypos=STATUS_AREA_PX+(fheight+1)*Mon_image.nparams+2*fheight;
      drawRectangle(1,ypos-fheight,dw,fheight-1,_WHITE);
      os_draw_string_small(1,ypos-fheight,_BLACK,_WHITE,ch);
      if (Mon_image.pushed && Mon_image.moving_param){
	drawLine(64,ypos-2,192,ypos-2,is3d?_WHITE:_BLACK);
	drawLine(64,ypos,64,ypos-fheight,is3d?_WHITE:_BLACK);
	drawLine(192,ypos,192,ypos-fheight,is3d?_WHITE:_BLACK);
	os_draw_string_small(65,ypos-fheight-2,_WHITE,_BLACK,f[1].print(contextptr).c_str());
	os_draw_string_small(193,ypos-fheight-2,_WHITE,_BLACK,f[2].print(contextptr).c_str());
	gen gxpos=64+128*(g-f[1])/(f[2]-f[1]);
	if (gxpos.type==_DOUBLE_){
	  int xpos=gxpos._DOUBLE_val;
	  drawLine(xpos,ypos,xpos,ypos-fheight,_RED);
	}
      }
    } 
    string the_legend;
    vecteur style(get_style(f,the_legend));
    int styles=style.size();
    // color
    int ensemble_attributs = style.front().val;
    bool hidden_name = false;
    if (style.front().type==_ZINT){
      ensemble_attributs = mpz_get_si(*style.front()._ZINTptr);
      hidden_name=true;
    }
    else
      hidden_name=ensemble_attributs<0;
    int width           =(ensemble_attributs & 0x00070000) >> 16; // 3 bits
    int epaisseur_point =(ensemble_attributs & 0x00380000) >> 19; // 3 bits
    int type_line       =(ensemble_attributs & 0x01c00000) >> 22; // 3 bits
    if (type_line>4)
      type_line=(type_line-4)<<8;
    int type_point      =(ensemble_attributs & 0x0e000000) >> 25; // 3 bits
    int labelpos        =(ensemble_attributs & 0x30000000) >> 28; // 2 bits
    bool fill_polygon   =(ensemble_attributs & 0x40000000) >> 30;
    int couleur         =(ensemble_attributs & 0x0007ffff);
    epaisseur_point += 2;
    if (s==at_pnt){ 
      // f[0]=complex pnt or vector of complex pnts or symbolic
      // f[1] -> style 
      // f[2] optional=label
      gen point=f[0];
      if (point.type==_VECT && point.subtype==_POINT__VECT)
	return;
      if ( (f[0].type==_SYMB) && (f[0]._SYMBptr->sommet==at_curve) && (f[0]._SYMBptr->feuille.type==_VECT) && (f[0]._SYMBptr->feuille._VECTptr->size()) ){
	// Mon_image.show_mouse_on_object=false;
	point=f[0]._SYMBptr->feuille._VECTptr->back();
	if (type_line>=4 && point.type==_VECT && point._VECTptr->size()>2){
	  vecteur v=*point._VECTptr;
	  int vs=v.size()/2; // 3 -> 1
	  if (Mon_image.findij(v[vs],x_scale,y_scale,i0,j0,contextptr) && Mon_image.findij(v[vs+1],x_scale,y_scale,i1,j1,contextptr)){
	    bool logx=Mon_image.display_mode & 0x400,logy=Mon_image.display_mode & 0x800;
	    checklog_fl_line(i0,j0,i1,j1,deltax,deltay,logx,logy,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	    double dx=i0-i1,dy=j0-j1;
	    petite_fleche(i1,j1,dx,dy,deltax,deltay,width+3,couleur);
	  }
	}
      }
      if (is_undef(point))
	return;
      // fl_line_style(type_line,width+1,0); 
      if (point.type==_SYMB) {
	if (point._SYMBptr->sommet==at_cercle){
	  vecteur v=*point._SYMBptr->feuille._VECTptr;
	  gen diametre=remove_at_pnt(v[0]);
	  gen e1=diametre._VECTptr->front().evalf_double(1,contextptr),e2=diametre._VECTptr->back().evalf_double(1,contextptr);
	  gen centre=rdiv(e1+e2,2.0,contextptr);
	  gen e12=e2-e1;
	  double ex=evalf_double(re(e12,contextptr),1,contextptr)._DOUBLE_val,ey=evalf_double(im(e12,contextptr),1,contextptr)._DOUBLE_val;
	  if (!Mon_image.findij(centre,x_scale,y_scale,i0,j0,contextptr))
	    return;
	  gen diam=std::sqrt(ex*ex+ey*ey);
	  gen angle=std::atan2(ey,ex);
	  gen a1=v[1].evalf_double(1,contextptr),a2=v[2].evalf_double(1,contextptr);
	  bool full=v[1]==0 && v[2]==cst_two_pi;
	  if ( (diam.type==_DOUBLE_) && (a1.type==_DOUBLE_) && (a2.type==_DOUBLE_) ){
	    i1=diam._DOUBLE_val*x_scale/2.0;
	    j1=diam._DOUBLE_val*y_scale/2.0;
            if (i1>MAX_DISP_RADIUS || j1>MAX_DISP_RADIUS)
              return;
	    double a1d=a1._DOUBLE_val,a2d=a2._DOUBLE_val,angled=angle._DOUBLE_val;
	    bool changer_sens=a1d>a2d;
	    if (changer_sens){
	      double tmp=a1d;
	      a1d=a2d;
	      a2d=tmp;
	    }
	    double anglei=(angled+a1d),anglef=(angled+a2d),anglem=(anglei+anglef)/2;
	    //confirm(full?"full":"partial",print_DOUBLE_(a1d,3).c_str())
	    if (fill_polygon)
	      fl_pie(deltax+round(i0-i1),deltay+round(j0-j1),round(2*i1),round(2*j1),full?0:anglei*180/M_PI+.5,full?360:anglef*180/M_PI+.5,couleur,false);
	    else {
	      fl_arc(deltax+round(i0-i1),deltay+round(j0-j1),round(2*i1),round(2*j1),full?0:anglei*180/M_PI+.5,full?360:anglef*180/M_PI+.5,couleur);
	      if (v.size()>=4){ // if cercle has the optionnal 5th arg
		if (v[3]==2)
		  petite_fleche(i0+i1*std::cos(anglem),j0-j1*std::sin(anglem),-i1*std::sin(anglem),-j1*std::cos(anglem),deltax,deltay,width,couleur);
		else {
		  if (changer_sens)
		    petite_fleche(i0+i1*std::cos(anglei),j0-j1*std::sin(anglei),-i1*std::sin(anglei),-j1*std::cos(anglei),deltax,deltay,width,couleur);
		  else
		    petite_fleche(i0+i1*std::cos(anglef),j0-j1*std::sin(anglef),i1*std::sin(anglef),j1*std::cos(anglef),deltax,deltay,width,couleur);
		}
	      }
	    }
	    // Label a few degrees from the start angle, 
	    // FIXME should use labelpos
	    double anglel=angled+a1d+0.3;
	    if (v.size()>=4 && v[3]==2)
	      anglel=angled+(0.45*a1d+0.55*a2d);
	    i0=i0+i1*std::cos(anglel); 
	    j0=j0-j1*std::sin(anglel);
	    if (!hidden_name)
	      draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur);
	    return;
	  }
	} // end circle
#if 1
	if (point._SYMBptr->sommet==at_legende){
	  gen & f=point._SYMBptr->feuille;
	  if (f.type==_VECT && f._VECTptr->size()==3){
	    vecteur & fv=*f._VECTptr;
	    if (fv[0].type==_VECT && fv[0]._VECTptr->size()>=2 && fv[1].type==_STRNG && fv[2].type==_INT_){
	      vecteur & fvv=*fv[0]._VECTptr;
	      if (fvv[0].type==_INT_ && fvv[1].type==_INT_){
		int dx=0,dy=0;
		string legendes(*fv[1]._STRNGptr);
		find_dxdy(legendes,labelpos,fontsize,dx,dy);
		text_print(fontsize,legendes.c_str(),deltax+fvv[0].val+dx,deltay+fvv[1].val+dy,fv[2].val);
	      }
	    }
	  }
	}
#endif
      } // end point.type==_SYMB
      if (point.type!=_VECT || (point.type==_VECT && (point.subtype==_GROUP__VECT || point.subtype==_VECTOR__VECT) && point._VECTptr->size()==2 && is_zero(point._VECTptr->back()-point._VECTptr->front())) ){ // single point
	if (!Mon_image.findij((point.type==_VECT?point._VECTptr->front():point),x_scale,y_scale,i0,j0,contextptr))
	  return;
	if (i0>0 && i0<mxw && j0>0 && j0<myw)
	  fltk_point(deltax,deltay,round(i0),round(j0),epaisseur_point,type_point,couleur);
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur);
	return;
      }
      // path
      const_iterateur jt=point._VECTptr->begin(),jtend=point._VECTptr->end();
      if (jt==jtend)
	return;
      bool logx=Mon_image.display_mode & 0x400,logy=Mon_image.display_mode & 0x800;
      if (jt->type==_VECT)
	return;
      if ( (type_point || epaisseur_point>2) && type_line==0 && width==0){
	for (;jt!=jtend;++jt){
	  if (!Mon_image.findij(*jt,x_scale,y_scale,i0,j0,contextptr))
	    return;
	  if (i0>0 && i0<mxw && j0>0 && j0<myw)
	    fltk_point(deltax,deltay,round(i0),round(j0),epaisseur_point,type_point,couleur);
	}
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur);
	return;
      }
      // initial point
      if (!Mon_image.findij(*jt,x_scale,y_scale,i0,j0,contextptr))
	return;
      i0save=i0;
      j0save=j0;
      if (fill_polygon){
	if (jtend-jt==5 && *(jt+4)==*jt){
	  // check rectangle parallel to axes -> draw_rectangle (filled)
	  int cote1=horiz_or_vert(jt,contextptr);
	  if (cote1 && horiz_or_vert(jt+1,contextptr)==3-cote1 && horiz_or_vert(jt+2,contextptr)==cote1 && horiz_or_vert(jt+3,contextptr)==3-cote1){
	    if (!Mon_image.findij(*(jt+2),x_scale,y_scale,i0,j0,contextptr))
	      return;
	    int x,y,w,h;
	    if (i0<i0save){
	      x=i0;
	      w=i0save-i0;
	    }
	    else {
	      x=i0save;
	      w=i0-i0save;
	    }
	    if (j0<j0save){
	      y=j0;
	      h=j0save-j0;
	    }
	    else {
	      y=j0save;
	      h=j0-j0save;
	    }
	    draw_rectangle(deltax+x,deltay+y,w,h,couleur);
	    return;
	  }
	} // end rectangle check
	bool closed=*jt==*(jtend-1);
	vector< vector<int> > vi(jtend-jt+(closed?0:1),vector<int>(2));
	for (int pos=0;jt!=jtend;++pos,++jt){
	  if (!Mon_image.findij(*jt,x_scale,y_scale,i0,j0,contextptr))
	    return;
	  vi[pos][0]=i0+deltax;
	  vi[pos][1]=j0+deltay;
	}
	if (!closed)
	  vi.back()=vi.front();
	draw_filled_polygon(vi,0,LCD_WIDTH_PX,24,LCD_HEIGHT_PX,couleur);
	return;
      }
      ++jt;
      if (jt==jtend){
	if (i0>0 && i0<mxw && j0>0 && j0<myw)
	  check_fl_point(deltax+round(i0),deltay+round(j0),clip_x,clip_y,clip_w,clip_h,0,0,couleur);
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur);
	return;
      }
      bool seghalfline=( point.subtype==_LINE__VECT || point.subtype==_HALFLINE__VECT ) && (point._VECTptr->size()==2);
      // rest of the path
      for (;;){
	if (!Mon_image.findij(*jt,x_scale,y_scale,i1,j1,contextptr))
	  return;
	if (!seghalfline){
	  checklog_fl_line(i0,j0,i1,j1,deltax,deltay,logx,logy,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	  if (point.subtype==_VECTOR__VECT){
	    double dx=i0-i1,dy=j0-j1;
	    petite_fleche(i1,j1,dx,dy,deltax,deltay,width,couleur);
	  }
	}
	++jt;
	if (jt==jtend){ // label of line at midpoint
	  if (point.subtype==_LINE__VECT){
	    i0=(6*i1-i0)/5-8;
	    j0=(6*j1-j0)/5-8;
	  }
	  else {
	    i0=(i0+i1)/2-8;
	    j0=(j0+j1)/2;
	  }
	  break;
	}
	i0=i1;
	j0=j1;
      }
      // check for a segment/halfline/line
      if ( seghalfline){
	double deltai=i1-i0save,adeltai=absdouble(deltai);
	double deltaj=j1-j0save,adeltaj=absdouble(deltaj);
	if (point.subtype==_LINE__VECT){
	  if (deltai==0)
	    checklog_fl_line(i1,0,i1,clip_h,deltax,deltay,logx,logy,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	  else {
	    if (deltaj==0)
	      checklog_fl_line(0,j1,clip_w,j1,deltax,deltay,Mon_image.display_mode & 0x400,Mon_image.display_mode & 0x800,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	    else {
	      // Find the intersections with the 4 rectangle segments
	      // Horizontal x=0 or w =i1+t*deltai: y=j1+t*deltaj
	      vector< complex<double> > pts;
	      double y0=j1-i1/deltai*deltaj,tol=clip_h*1e-6;
	      if (y0>=-tol && y0<=clip_h+tol)
		pts.push_back(complex<double>(0.0,y0));
	      double yw=j1+(clip_w-i1)/deltai*deltaj;
	      if (yw>=-tol && yw<=clip_h+tol)
		pts.push_back(complex<double>(clip_w,yw));
	      // Vertical y=0 or h=j1+t*deltaj, x=i1+t*deltai
	      double x0=i1-j1/deltaj*deltai;
	      tol=clip_w*1e-6;
	      if (x0>=-tol && x0<=clip_w+tol)
		pts.push_back(complex<double>(x0,0.0));
	      double xh=i1+(clip_h-j1)/deltaj*deltai;
	      if (xh>=-tol && xh<=clip_w+tol)
		pts.push_back(complex<double>(xh,clip_h));
	      if (pts.size()>=2)
		checklog_fl_line(pts[0].real(),pts[0].imag(),pts[1].real(),pts[1].imag(),deltax,deltay,Mon_image.display_mode & 0x400,Mon_image.display_mode & 0x800,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	    } // end else adeltai==0 , adeltaj==0
	  } // end else adeltai==0
	} // end LINE_VECT
	else {
	  double N=1;
	  if (adeltai){
	    N=clip_w/adeltai+1;
	    if (adeltaj)
	      N=max(N,clip_h/adeltaj+1);
	  }
	  else {
	    if (adeltaj)
	      N=clip_h/adeltaj+1;
	  }
	  N *= 2; // increase N since rounding might introduce too small clipping
	  while (fabs(N*deltai)>10000)
	    N /= 2;
	  while (fabs(N*deltaj)>10000)
	    N /= 2;
	  checklog_fl_line(i0save,j0save,i1+N*deltai,j1+N*deltaj,deltax,deltay,Mon_image.display_mode & 0x400,Mon_image.display_mode & 0x800,Mon_image.window_xmin,x_scale,Mon_image.window_ymax,y_scale,couleur);
	}
      } // end seghalfline
      if ( (point.subtype==_GROUP__VECT) && (point._VECTptr->size()==2))
	; // no legend for segment
      else {
	if (!hidden_name)
	  draw_legende(f,round(i0),round(j0),labelpos,&Mon_image,clip_x,clip_y,clip_w,clip_h,0,0,couleur);
      }
    } // end pnt subcase
  }
#endif

  // return a vector of values with simple decimal representation
  // between xmin/xmax or including xmin/xmax (if bounds is true)
  vecteur ticks(double xmin,double xmax,bool bounds){
    if (xmax<xmin)
      swapdouble(xmin,xmax);
    double dx=xmax-xmin;
    vecteur res;
    if (dx==0)
      return res;
    double d=std::pow(10.0,std::floor(std::log10(dx)));
    if (dx<2*d)
      d=d/5;
    else {
      if (dx<5*d)
	d=d/2;
    }
    double x1=std::floor(xmin/d)*d;
    double x2=(bounds?std::ceil(xmax/d):std::floor(xmax/d))*d;
    for (double x=x1+(bounds?0:d);x<=x2;x+=d){
      if (absdouble(x-int(x+.5))<1e-6*d)
	res.push_back(int(x+.5));
      else
	res.push_back(x);
    }
    return res;
  }

  void normalize(double & x, double & y){
    double fx=fabs(x),fy=fabs(y);
    if (fx>fy){
      y/=fx;
      x/=fx;
    }
    else {
      x/=fy;
      y/=fy;
    }
    double n=std::sqrt(x*x+y*y);
    x/=n; y/=n;
  }

  void Graph2d::adddepth(vector<int2> & polyg,const float3d3 &A,const float3d3 &B,int2 & IJmin) const {
    if ((A.z-current_depth)*(B.z-current_depth)>0)
      return;
    double t=(current_depth-A.z)/(B.z-A.z);
    double x=A.x+t*(B.x-A.x);
    double y=A.y+t*(B.y-A.y);
    int I,J;
    XYZ2ij(float3d3(x,y,current_depth),I,J);
    int2 IJ(I,J);
    polyg.push_back(IJ);
    if (IJ<IJmin)
      IJmin=IJ;
  }

  void Graph2d::addpolyg(vector<int2> & polyg,float3d x,float3d y,float3d z,int2 & IJmin) const {
    int I,J;
    xyz2ij(float3d3(x,y,z),I,J);
    int2 IJ(I,J);
    polyg.push_back(IJ);
    if (IJ<IJmin)
      IJmin=IJ;
  }

  int roundint (double r) {
    int tmp = static_cast<int> (r);
    tmp += (r-tmp>=.5) - (r-tmp<=-.5);
    return tmp;
  }

  bool Graph2d::find_dxdy(double & dx, double & dy) const {
    double xmin=window_xmin,xmax=window_xmax;
    int hp=LCD_WIDTH_PX-1;
    dx=(xmax-xmin)/hp;
    double ymin=window_ymin,ymax=window_ymax;
    int vp=LCD_HEIGHT_PX-STATUS_AREA_PX-1;
    dy=(ymax-ymin)/vp;
    return abs(dx-dy) < 0.000001;
  }

  void Graph2d::find_xy(double i,double j,double & x,double & y) const {
    double xmin=window_xmin,xmax=window_xmax;
    x=xmin+i*(xmax-xmin)/LCD_WIDTH_PX;
    double ymin=window_ymin,ymax=window_ymax;
    y=ymax-j*(ymax-ymin)/(LCD_HEIGHT_PX-STATUS_AREA_PX);
  }

  void Graph2d::round_xy(double & x, double & y) const {
    double dx,dy;
    find_dxdy(dx,dy);
    double range = std::pow(10,std::log10(1.0/dx));
    x = roundint(x * range) / range;
    y = roundint(y * range) / range;
  }
  
  void round3(double & x,double xmin,double xmax){
    double dx=std::abs(xmax-xmin);
    double logdx=std::log10(dx);
    int ndec=int(logdx)-3;
    double xpow=std::pow(10.0,ndec);
    int newx=x>=0?int(x/xpow+0.5):int(x/xpow-0.5);
    x=newx*xpow;
  }

  vecteur Graph2d::param(double d) const {
    const_iterateur it=plot_instructions.begin(),itend=plot_instructions.end();
    vecteur res;
    double pos=0.5;
    for (int i=0 ;it!=itend;++i,++it){
      gen tmp=*it;
      if (tmp.is_symb_of_sommet(at_parameter)){
	tmp=tmp._SYMBptr->feuille;
	if (tmp.type==_VECT && tmp._VECTptr->size()>=4){
	  if (std::abs(d-pos)<0.50001){
	    res.push_back(tmp);
	    res.push_back(i);
	    //confirm(tmp.print().c_str(),print_INT_(i).c_str());
	    return res;
	  }
	  ++pos;
	}
      }
    }
    return res;
  }
  
  void Graph2d::draw_decorations(const gen & title_tmp){
    if (args_tmp.empty()){ // add selected names
      char s[256]; strcpy(s,modestr.c_str());
      int pos=0,modestrsize=modestr.size();
      pos += modestrsize;
      s[pos++]=' ';
      if (mode!=0 && drag_name.type==_IDNT){
	strcpy(s+pos,drag_name._IDNTptr->id_name);
	pos += strlen(drag_name._IDNTptr->id_name);
      }
      else {
	if (1 || mode==0 || mode==255){ // print selected names 
	  vecteur v;
	  if (mode!=255) v=selected_names(true,false);
	  if (v.empty() && current_i<=192 && current_j<14*nparams+21){
	    //confirm(print_INT_(current_j).c_str(),"");
	    double d=current_j/14.-2;
	    v=param(d);
	    if (v.size()!=2)
	      v.clear();
	    else
	      v=vecteur(1,v.front()[0]);
	  }
	  int vs=v.size();
	  if (!vs){ // print current coordinates
	    double i=current_i,j=current_j,x,y,z;
	    if (is3d){
	      find_xyz(i,j,current_depth,x,y,z);
	      round_xy(x,y); round3(z,window_zmin,window_zmax);
	      string tmp=giac::print_DOUBLE_(x,3)+","+giac::print_DOUBLE_(y,3)+","+giac::print_DOUBLE_(z,3);
	      strcpy(s+pos,tmp.c_str()); // sprintf(s+pos," %.3g,%.3g,%.3g",x,y,z);
	    }
	    else {
	      find_xy(i,j,x,y);
	      // round to maximum pixel range
	      round_xy(x,y);
	      string tmp="x="+giac::print_DOUBLE_(x,3)+",y="+giac::print_DOUBLE_(y,3);
	      strcpy(s+pos,tmp.c_str()); // sprintf(s+pos," %.3g,%.3g",x,y);
	    }
	    pos=strlen(s);
	  }
	  for (int i=0;i<vs && pos<100;++i){
	    if (v[i].type==_IDNT){
	      strcpy(s+pos,v[i]._IDNTptr->id_name);
	      pos += strlen(v[i]._IDNTptr->id_name);
	      if (i<vs-1){
		s[pos++]=',';
	      }
	    }
	  }
	}
      }
      s[pos]=0;
      if (tracemode){
	if (tracemode_add.size())
	  os_draw_string_small(0,24,_WHITE,_BLACK,tracemode_add.c_str());
	else
	  os_draw_string_small(LCD_WIDTH_PX-fl_width(s),24,_WHITE,_BLACK,s);	  
	fltk_draw(*this,tracemode_disp,x_scale,y_scale,0,0,LCD_WIDTH_PX,LCD_HEIGHT_PX);
      }
      else
	os_draw_string_small(LCD_WIDTH_PX-20-os_draw_string_small(0,0,_WHITE,_BLACK,s,true),tracemode?24:LCD_HEIGHT_PX-14,_WHITE,_BLACK,s);
    }
    if (mode && title.size()<100 && (!title.empty() || !is_zero(title_tmp))){
      std::string mytitle;
      if (!is_zero(title_tmp) && function_final.type==_FUNC){
	if (function_final==at_point && title_tmp.is_symb_of_sommet(at_point))
	  mytitle=title_tmp.print(contextptr);
	else
	  mytitle=function_final._FUNCptr->ptr()->s+('('+title_tmp.print(contextptr)+')'); // gen(symbolic(*function_final._FUNCptr,title_tmp)).print(contextptr);
      }
      else
	mytitle=title;
      if (!mytitle.empty()){
	int dt=int(fl_width(mytitle.c_str()));
	if (dt>LCD_WIDTH_PX)
	  dt=LCD_WIDTH_PX;
	os_draw_string_small((LCD_WIDTH_PX-dt)/2,LCD_HEIGHT_PX-14,_WHITE,_BLACK,mytitle.c_str());
      }
    }
    if (hp || tracemode){ // draw cursor at current_i,current_j
      int taille=(mode==255 && !tracemode) ?2:5;
      int j=current_j;
      if (!is3d) j += STATUS_AREA_PX;
      fl_line(current_i-taille,j,current_i+taille,j,is3d?_CYAN:_BLUE);
      fl_line(current_i,j-taille,current_i,j+taille,is3d?_CYAN:_BLUE);
      if (cursor_point_type==6){
	fl_line(current_i-2,j+2,current_i+2,j+2,_RED);
	fl_line(current_i-2,j+2,current_i+2,j+2,_RED);
	fl_line(current_i-2,j-2,current_i-2,j+2,_RED);
	fl_line(current_i+2,j-2,current_i+2,j+2,_RED);
      }
    }
  }

  void displaypolyg(const vector<int2> & polyg,const int2 & IJmin,int color,int & Px,int & Py,GIAC_CONTEXT){
    if (polyg.empty())
      return;
    // sort list of arguments
    vector<int2_double2> p;
    for (int k=0;k<polyg.size();++k){
      const int2 & cur=polyg[k];
      if (cur==IJmin){
	int2_double2 id={cur.i,cur.j,0,0};
	p.push_back(id);
      } else {
	double di=cur.i-IJmin.i,dj=cur.j-IJmin.j;
	int2_double2 id={cur.i,cur.j,std::atan2(di,dj),di*di+dj*dj};
	p.push_back(id);
      }
    }
    sort(p.begin(),p.end());
    // draw polygon
    vector< vector<int> > P;
    for (int k=0;k<p.size();++k){
      vector<int> vi(2);
      vi[0]=p[k].i;
      vi[1]=p[k].j;
      P.push_back(vi);
    }
    draw_polygon(P,color );
    Px=P[0][0];
    Py=P[0][1];
  }

  void Graph2d::draw(){
    nparams=0;
    if (is3d){
      //cout << "gr x" << window_xmin << " " << window_xmax << "\n";
      //cout << "gr y" << window_ymin << " " << window_ymax << "\n";
      //cout << "gr z" << window_zmin << " " << window_zmax << "\n";
      if (lang==1)
	statuslinemsg("DEL: stop");
      else
	statuslinemsg("DEL: stop");
      float3d3 A(window_xmin,window_ymin,window_zmin),
	B(window_xmin,window_ymin,window_zmax),
	C(window_xmax,window_ymin,window_zmin),
	D(window_xmax,window_ymin,window_zmax),
	E(window_xmin,window_ymax,window_zmin),
	F(window_xmin,window_ymax,window_zmax),
	G(window_xmax,window_ymax,window_zmin),
	H(window_xmax,window_ymax,window_zmax);
      float3d3 A3,B3,C3,D3,E3,F3,G3,H3;
      xyz2ij(A,Ai,Aj,A3);
      xyz2ij(B,Bi,Bj,B3);
      xyz2ij(C,Ci,Cj,C3);
      xyz2ij(D,Di,Dj,D3);
      xyz2ij(E,Ei,Ej,E3);
      xyz2ij(F,Fi,Fj,F3);
      xyz2ij(G,Gi,Gj,G3);
      xyz2ij(H,Hi,Hj,H3);
      set_abort();
      int prec=precision;
      if (mode==0 && precision<3)
	precision += 2;
      //do_confirm("draw 2");
      glsurface(precision,precision,lcdz,contextptr,default_upcolor,default_downcolor,default_downupcolor,default_downdowncolor);
      //do_confirm("draw 3");
      precision=prec;
      clear_abort();
      if (show_edges){
	// polyhedrons
	for (int k=0;k<int(polyedrev.size());++k){
	  const vector<float3d3> & cur=polyedrev[k]; // current face
	  const int4 & col=polyedre_color[k];
	  for (int l=1;l<int(cur.size());++l){
	    const float3d3 & p=cur[l?l-1:cur.size()-1];
	    const float3d3 & c=cur[l];
	    // is edge visible?
	    float3d3 m(p.x/2+c.x/2,p.y/2+c.y/2,p.z/2+c.z/2);
	    double xy=m.x+m.y;
	    int mi,mj;
	    XYZ2ij(m,mi,mj);
	    int kk,jmin=RAND_MAX,jmax=-RAND_MAX;
	    for (kk=0;kk<int(polyedrev.size());++kk){
	      if (k==kk)
		continue;
	      const vector<float3d3> & Cur=polyedrev[kk];
	      int ll;
	      // first check if point is in face
	      for (ll=1;ll<int(Cur.size());++ll){
		const float3d3 & P=Cur[ll?ll-1:Cur.size()-1];
		const float3d3 & C=Cur[ll];
		float3d3 M(P.x/2+C.x/2,P.y/2+C.y/2,P.z/2+C.z/2);
		if (M.x==m.x && M.y==m.y && M.z==m.z){
		  break; // edge PC has same midpoint, will ignore face
		}
	      }
	      if (ll<int(Cur.size())) // point is in face, ignore face
		continue;
	      float3d3 M0; bool found1st=false;
	      for (ll=1;ll<int(Cur.size());++ll){
		const float3d3 & P=Cur[ll?ll-1:Cur.size()-1];
		const float3d3 & C=Cur[ll];
		// intersect plane y-x=m.y-m.x with PC edge P+t*PC
		double PCx=C.x-P.x,PCy=C.y-P.y,dPC=PCy-PCx;
		// P.y-P.x + t*dPC=m.y-m.x
		if (dPC==0) // edge is parallel
		  continue;
		double t=((m.y-m.x)+(P.x-P.y))/dPC;
		if (t<0 || t>1)
		  continue;
		double x=P.x+t*PCx;
		double y=P.y+t*PCy;
		double z=P.z+t*(C.z-P.z);
		if (!found1st){
		  M0=float3d3(x,y,z);
		  found1st=true;
		  continue;
		}
		if (x==M0.x && y==M0.y && z==M0.z)
		  continue;
		// segment([x,y,z],M0) has same y-x as m,
		// find segment position for same y+x as m [x,y,z]+t*(M0-[x,y,z])
		// yx=x+y+t*(M0.x-x+M0.y-y)
		double M0xy=M0.x-x+M0.y-y;
		int i1,j1,i2,j2; // N.B. i1,i2 should be the same as mi
		if (abs(M0xy)<1e-14)
		  t=-1;
		else
		  t=(xy-x-y)/M0xy;
		if (t<=0 || t>=1){
		  if (x+y<=xy) // segment is behind midpoint m
		    continue;
		  XYZ2ij(M0,i1,j1); 
		  XYZ2ij(float3d3(x,y,z),i2,j2);
		  if (j1>j2) swapint(j1,j2);
		  if (jmin>j1) jmin=j1;
		  if (jmax<j2) jmax=j2;
		  if (jmin<mj && mj<jmax){
		    break;
		  }
		  continue;
		}
		// find segment part that might mask midpoint m
		double X = x+t*(M0.x-x);
		double Y = y+t*(M0.y-y);
		double Z = z+t*(M0.z-z);
		XYZ2ij(float3d3(X,Y,Z),i1,j1);
		if (x+y<=xy)
		  XYZ2ij(M0,i2,j2);
		else
		  XYZ2ij(float3d3(x,y,z),i2,j2);
		if (j1>j2) swapint(j1,j2);
		if (jmin>j1) jmin=j1;
		if (jmax<j2) jmax=j2;
		if (jmin<mj && mj<jmax){
		  break;
		}
	      } // end for
	      if (ll<int(Cur.size())){
		// means edge is not visible
		break;
	      }
	    }
	    // polyedre attribute: filled/not filled
	    bool filled=polyedre_filled[k];
	    bool hidden=kk<int(polyedrev.size());
	    if (filled && hidden)
	      continue;
	    int i1,j1,i2,j2;
	    XYZ2ij(p,i1,j1);
	    XYZ2ij(c,i2,j2);
	    if (i1>i2 || (i1==i2 && j1>j2)){
	      swapint(i1,i2); swapint(j1,j2);
	    }
	    drawLine(i1,j1,i2,j2,
		     // col.d | 0x400000
		     col.u | ((hidden || filled)?0x400000:0)
		     );
	  }
	}
      }
      if (show_axes){
	// cube A,B,C,D,E,F,G,H
	// X
	drawLine(Ai,Aj,Ci,Cj,COLOR_RED | 0x800000);
	drawLine(Bi,Bj,Di,Dj,COLOR_RED | 0x800000);
	drawLine(Ei,Ej,Gi,Gj,COLOR_RED | 0x800000);
	drawLine(Fi,Fj,Hi,Hj,COLOR_RED | 0x800000);
	// Y
	drawLine(Ai,Aj,Ei,Ej,COLOR_GREEN | 0x800000);
	drawLine(Bi,Bj,Fi,Fj,COLOR_GREEN | 0x800000);
	drawLine(Ci,Cj,Gi,Gj,COLOR_GREEN | 0x800000);
	drawLine(Di,Dj,Hi,Hj,COLOR_GREEN | 0x800000);
	// Z
	drawLine(Ai,Aj,Bi,Bj,_CYAN | 0x800000);
	drawLine(Ci,Cj,Di,Dj,_CYAN | 0x800000);
	drawLine(Ei,Ej,Fi,Fj,_CYAN | 0x800000);
	drawLine(Gi,Gj,Hi,Hj,_CYAN | 0x800000);
	// current_depth
	if (hp){
	  vector<int2> polyg; int2 IJmin={RAND_MAX,RAND_MAX};
	  // x: A3-C3, B3-D3; E3-G3,F3-H3
	  adddepth(polyg,A3,C3,IJmin);
	  adddepth(polyg,B3,D3,IJmin);
	  adddepth(polyg,E3,G3,IJmin);
	  adddepth(polyg,F3,H3,IJmin);
	  // y: A3-E3; B3-F3; C3-G3, D3-H3
	  adddepth(polyg,A3,E3,IJmin);
	  adddepth(polyg,B3,F3,IJmin);
	  adddepth(polyg,C3,G3,IJmin);
	  adddepth(polyg,D3,H3,IJmin);
	  // z: A3-B3, C3-D3, E3-F3, G3-H3
	  adddepth(polyg,A3,B3,IJmin);
	  adddepth(polyg,C3,D3,IJmin);
	  adddepth(polyg,E3,F3,IJmin);
	  adddepth(polyg,G3,H3,IJmin);
	  int Px,Py;
	  displaypolyg(polyg,IJmin,COLOR_YELLOW | 0x400000,Px,Py,contextptr);
	}
	// planes
	vecteur attrv(gen2vecteur(g));
	for (int i=0;i<attrv.size();++i){
	  gen attr=attrv[i];
	  gen cur=remove_at_pnt(attr);
	  int upcolor=44444;
	  const char * nameptr=0;
	  if (attr.is_symb_of_sommet(at_pnt)){
	    if (show_names && attr._SYMBptr->feuille.type==_VECT && attr._SYMBptr->feuille._VECTptr->size()==3){
	      gen name=attr._SYMBptr->feuille._VECTptr->back();
	      if (name.type==_IDNT)
		nameptr=name._IDNTptr->id_name;
	      if (name.type==_STRNG)
		nameptr=name._STRNGptr->c_str();
	    }
	    attr=attr._SYMBptr->feuille[1];
	    if (attr.type==_INT_ && (attr.val & 0xffff)!=0){
	      upcolor=attr.val &0xffff;
	    }
	  }
	  if (cur.is_symb_of_sommet(at_hyperplan)){
	    vecteur & w=*cur._SYMBptr->feuille._VECTptr;
	    gen m=evalf_double(w[1],1,contextptr),n=evalf_double(w[0],1,contextptr);
	    double a=n[0]._DOUBLE_val,b=n[1]._DOUBLE_val,c=n[2]._DOUBLE_val;
	    double x0=m[0]._DOUBLE_val,y0=m[1]._DOUBLE_val,z0=m[2]._DOUBLE_val;
	    // a*(x-x0)+b*(y-y0)+c*(z-z0)=0
	    // replace 2 coordinates of M with window_xyzminmax and find last coord
	    vector<int2> polyg; int2 IJmin={RAND_MAX,RAND_MAX};
	    // x
	    if (a!=0){
	      double x=x0-1/a*(b*(window_ymin-y0)+c*(window_zmin-z0));
	      if (x>=window_xmin && x<=window_xmax)
		addpolyg(polyg,x,window_ymin,window_zmin,IJmin);
	      x=x0-1/a*(b*(window_ymin-y0)+c*(window_zmax-z0));
	      if (x>=window_xmin && x<=window_xmax)
		addpolyg(polyg,x,window_ymin,window_zmax,IJmin);
	      x=x0-1/a*(b*(window_ymax-y0)+c*(window_zmin-z0));
	      if (x>=window_xmin && x<=window_xmax)
		addpolyg(polyg,x,window_ymax,window_zmin,IJmin);
	      x=x0-1/a*(b*(window_ymax-y0)+c*(window_zmax-z0));
	      if (x>=window_xmin && x<=window_xmax)
		addpolyg(polyg,x,window_ymax,window_zmax,IJmin);
	    }
	    // y
	    if (b!=0){
	      double y=y0-1/b*(a*(window_xmin-x0)+c*(window_zmin-z0));
	      if (y>=window_ymin && y<=window_ymax)
		addpolyg(polyg,window_xmin,y,window_zmin,IJmin);
	      y=y0-1/b*(a*(window_xmin-x0)+c*(window_zmax-z0));
	      if (y>=window_ymin && y<=window_ymax)
		addpolyg(polyg,window_xmin,y,window_zmax,IJmin);
	      y=y0-1/b*(a*(window_xmax-x0)+c*(window_zmin-z0));
	      if (y>=window_ymin && y<=window_ymax)
		addpolyg(polyg,window_xmax,y,window_zmin,IJmin);
	      y=y0-1/b*(a*(window_xmax-x0)+c*(window_zmax-z0));
	      if (y>=window_ymin && y<=window_ymax)
		addpolyg(polyg,window_xmax,y,window_zmax,IJmin);
	    }
	    // z
	    if (c!=0){
	      double z=z0-1/c*(a*(window_xmin-x0)+b*(window_ymin-y0));
	      if (z>=window_zmin && z<=window_zmax)
		addpolyg(polyg,window_xmin,window_ymin,z,IJmin);
	      z=z0-1/c*(a*(window_xmin-x0)+b*(window_ymax-y0));
	      if (z>=window_zmin && z<=window_zmax)
		addpolyg(polyg,window_xmin,window_ymax,z,IJmin);
	      z=z0-1/c*(a*(window_xmax-x0)+b*(window_ymin-y0));
	      if (z>=window_zmin && z<=window_zmax)
		addpolyg(polyg,window_xmax,window_ymin,z,IJmin);
	      z=z0-1/c*(a*(window_xmax-x0)+b*(window_ymax-y0));
	      if (z>=window_zmin && z<=window_zmax)
		addpolyg(polyg,window_xmax,window_ymax,z,IJmin);
	    }
	    // sort list of arguments
	    vector<int2_double2> p;
	    for (int k=0;k<polyg.size();++k){
	      int2 & cur=polyg[k];
	      if (cur==IJmin){
		int2_double2 id={cur.i,cur.j,0,0};
		p.push_back(id);
	      } else {
		double di=cur.i-IJmin.i,dj=cur.j-IJmin.j;
		int2_double2 id={cur.i,cur.j,std::atan2(di,dj),di*di+dj*dj};
		p.push_back(id);
	      }
	    }
	    sort(p.begin(),p.end());
	    // draw polygon
	    vector< vector<int> > P;
	    for (int k=0;k<p.size();++k){
	      vector<int> vi(2);
	      vi[0]=p[k].i;
	      vi[1]=p[k].j;
	      P.push_back(vi);
	    }
	    draw_polygon(P,upcolor
			 // | 0x400000
			 );
	    if (nameptr){
	      int x=os_draw_string(0,0,0,upcolor,nameptr,true);
	      os_draw_string(P[0][0]-x,P[0][1],upcolor,0,nameptr);
	    }
	  }
	}
	// frame
	double xi=Ci-Ai,xj=Cj-Aj;
	normalize(xi,xj);
	int x0=350,y0=160+24;
	drawLine(x0,y0,x0+20*xi,y0+20*xj,COLOR_RED);
	os_draw_string(x0+20*xi,y0+20*xj,COLOR_RED,COLOR_BLACK,"x");
	double yi=Ei-Ai,yj=Ej-Aj;
	normalize(yi,yj);
	drawLine(x0,y0,x0+20*yi,y0+20*yj,COLOR_GREEN);
	os_draw_string(x0+20*yi,y0+20*yj,COLOR_GREEN,COLOR_BLACK,"y");
	double zi=Bi-Ai,zj=Bj-Aj;
	normalize(zi,zj);
	drawLine(x0,y0,x0+20*zi,y0+20*zj,COLOR_CYAN);
	os_draw_string(x0+20*zi,y0+20*zj,COLOR_CYAN,COLOR_BLACK,"z");
      } // end show_axes
      // now handle legend([x,y],string)
      vecteur V(gen2vecteur(g)); 
      for (int i=0;i<V.size();++i){
	gen attr=V[i];
	if (attr.is_symb_of_sommet(at_parameter) && attr._SYMBptr->feuille.type==_VECT){
	  vecteur f=*attr._SYMBptr->feuille._VECTptr;
	  int fs=f.size();
	  if (fs>=4 && f[0].type==_IDNT){
	    // display parameter from the left upper, f[0] name and f[3] value
	    char ch[128];
	    strcpy(ch,f[0]._IDNTptr->id_name);
	    int pos=strlen(ch);
	    ch[pos]='=';
	    ++pos;
	    ch[pos]=0;
	    gen g=evalf_double(f[3],1,contextptr);
	    if (g.type==_DOUBLE_)
	      strcpy(ch+pos,g.print(contextptr).c_str());
	    else {
	      ch[pos]='?';
	      ++pos;
	      ch[pos]=0;
	    }
	    ++nparams;
	    int dw=fl_width(ch);
	    int fheight=14;
	    int ypos=(fheight+1)*nparams+2*fheight;
	    drawRectangle(1,ypos-fheight,dw,fheight-1,_WHITE);
	    os_draw_string_small(1,ypos-fheight,_BLACK,_WHITE,ch);
	    if (pushed && moving_param){
	      drawLine(64,ypos-2,192,ypos-2,is3d?_WHITE:_BLACK);
	      drawLine(64,ypos,64,ypos-fheight,is3d?_WHITE:_BLACK);
	      drawLine(192,ypos,192,ypos-fheight,is3d?_WHITE:_BLACK);
	      os_draw_string_small(65,ypos-fheight-2,_WHITE,_BLACK,f[1].print(contextptr).c_str());
	      os_draw_string_small(193,ypos-fheight-2,_WHITE,_BLACK,f[2].print(contextptr).c_str());
	      gen gxpos=64+128*(g-f[1])/(f[2]-f[1]);
	      if (gxpos.type==_DOUBLE_){
		int xpos=gxpos._DOUBLE_val;
		drawLine(xpos,ypos,xpos,ypos-fheight,_RED);
	      }
	    }
	  }
	} // end parameter
	if (attr.is_symb_of_sommet(at_pnt)){
	  attr=attr._SYMBptr->feuille;
	  if (attr.type==_VECT && attr._VECTptr->size()>1){
	    int color=65535;
	    gen attr0=attr._VECTptr->front();
	    attr=attr[1];
	    if (attr.type==_INT_ && (attr.val & 0xffff)!=0){
	      color=attr.val &0xffff;
	    }
	    if (attr0.is_symb_of_sommet(at_legende)){
	      gen leg=attr0._SYMBptr->feuille;
	      if (leg.type==_VECT && leg._VECTptr->size()>=2){
		gen pos=leg._VECTptr->front();
		leg=leg[1];
		if (pos.type==_VECT && pos._VECTptr->size()==2 && leg.type==_STRNG){
		  gen x=pos._VECTptr->front(),y=pos._VECTptr->back();
		  if (x.type==_INT_ && y.type==_INT_)
		    os_draw_string(x.val,y.val,color,0,leg._STRNGptr->c_str());
		}
	      }
	    }
	  }
	}
	os_sync_screen();	
      }
      DefineStatusMessage((char*)"+-:zoom,F1:help,F6:menu,EXIT:quit", 1, 0, 0);
      DisplayStatusArea();
      if (hp || tracemode)
	draw_decorations(title_tmp);
      return;
    } // end is3d
    int save_clip_ymin=clip_ymin;
    clip_ymin=STATUS_AREA_PX;
    int horizontal_pixels=LCD_WIDTH_PX,vertical_pixels=LCD_HEIGHT_PX-STATUS_AREA_PX,deltax=0,deltay=STATUS_AREA_PX,clip_x=0,clip_y=0,clip_w=horizontal_pixels,clip_h=vertical_pixels;
    drawRectangle(0, STATUS_AREA_PX, horizontal_pixels, vertical_pixels,COLOR_WHITE);
    // Draw axis
    double I0,J0;
    findij(zero,x_scale,y_scale,I0,J0,contextptr); // origin
    int i_0=round(I0),j_0=round(J0);
    if (show_axes &&  (window_ymax>=0) && (window_ymin<=0)){ // X-axis
      vecteur aff; int affs;
      char ch[256];
      check_fl_line(deltax,deltay+j_0,deltax+horizontal_pixels,deltay+j_0,clip_x,clip_y,clip_w,clip_h,0,0,_GREEN); 
      check_fl_line(deltax+i_0,deltay+j_0,deltax+i_0+int(x_scale),deltay+j_0,clip_x,clip_y,clip_w,clip_h,0,0,_CYAN);
      aff=ticks(window_xmin,window_xmax,true);
      affs=aff.size();
      for (int i=0;i<affs;++i){
	double d=evalf_double(aff[i],1,contextptr)._DOUBLE_val;
	if (fabs(d)<1e-6) strcpy(ch,"0"); else sprint_double(ch,d);
	int delta=int(horizontal_pixels*(d-window_xmin)/(window_xmax-window_xmin));
	int taille=strlen(ch)*9;
	fl_line(delta,deltay+j_0,delta,deltay+j_0-4,_GREEN);
      }
      check_fl_draw(labelsize,"x",deltax+horizontal_pixels-40,deltay+j_0-4,clip_x,clip_y,clip_w,clip_h,0,0,_GREEN);
    }
    if ( show_axes && (window_xmax>=0) && (window_xmin<=0) ) {// Y-axis
      vecteur aff; int affs;
      char ch[256];
      check_fl_line(deltax+i_0,deltay,deltax+i_0,deltay+vertical_pixels,clip_x,clip_y,clip_w,clip_h,0,0,_RED);
      check_fl_line(deltax+i_0,deltay+j_0,deltax+i_0,deltay+j_0-int(y_scale),clip_x,clip_y,clip_w,clip_h,0,0,_CYAN);
      aff=ticks(window_ymin,window_ymax,true);
      affs=aff.size();
      int taille=5;
      for (int j=0;j<affs;++j){
	double d=evalf_double(aff[j],1,contextptr)._DOUBLE_val;
	if (fabs(d)<1e-6) strcpy(ch,"0"); else sprint_double(ch,d);
	int delta=int(vertical_pixels*(window_ymax-d)/(window_ymax-window_ymin));
	if (delta>=taille && delta<=vertical_pixels-taille){
	  fl_line(deltax+i_0,STATUS_AREA_PX+delta,deltax+i_0+4,STATUS_AREA_PX+delta,_RED);
	}
      }
      check_fl_draw(labelsize,"y",deltax+i_0+2,deltay+labelsize,clip_x,clip_y,clip_w,clip_h,0,0,_RED);
    }
#if 0 // if ticks are enabled, don't forget to set freeze to false
    // Ticks
    if (show_axes && (horizontal_pixels)/(x_scale*x_tick) < 40 && vertical_pixels/(y_tick*y_scale) <40  ){
      if (x_tick>0 && y_tick>0 ){
	double nticks=(horizontal_pixels-I0)/(x_scale*x_tick);
	double mticks=(vertical_pixels-J0)/(y_tick*y_scale);
	int count=0;
	for (int ii=int(-I0/(x_tick*x_scale));ii<=nticks;++ii){
	  int iii=int(I0+ii*x_scale*x_tick+.5);
	  for (int jj=int(-J0/(y_tick*y_scale));jj<=mticks && count<1600;++jj,++count){
	    int jjj=int(J0+jj*y_scale*y_tick+.5);
	    check_fl_point(deltax+iii,deltay+jjj,clip_x,clip_y,clip_w,clip_h,0,0,COLOR_BLACK);
	  }
	}
      }
    }
#endif
    if (show_axes){ 
      int taille,affs,delta;
      vecteur aff;
      char ch[256];
      // X
      aff=ticks(window_xmin,window_xmax,true);
      affs=aff.size();
      for (int i=0;i<affs;++i){
	double d=evalf_double(aff[i],1,contextptr)._DOUBLE_val;
	sprint_double(ch,d);
	delta=int(horizontal_pixels*(d-window_xmin)/(window_xmax-window_xmin));
	taille=strlen(ch)*9;
	fl_line(delta,vertical_pixels+STATUS_AREA_PX-6,delta,vertical_pixels+STATUS_AREA_PX-1,_GREEN);
	if (delta>=taille/2 && delta<=horizontal_pixels){
	  text_print(10,ch,delta-taille/2,vertical_pixels+STATUS_AREA_PX-7,_GREEN);
	}
      }
      // Y
      aff=ticks(window_ymin,window_ymax,true);
      affs=aff.size();
      taille=5;
      for (int j=0;j<affs;++j){
	double d=evalf_double(aff[j],1,contextptr)._DOUBLE_val;
	sprint_double(ch,d);
	delta=int(vertical_pixels*(window_ymax-d)/(window_ymax-window_ymin));
	if (delta>=taille && delta<=vertical_pixels-taille){
	  fl_line(horizontal_pixels-5,STATUS_AREA_PX+delta,horizontal_pixels-1,STATUS_AREA_PX+delta,_RED);
	  text_print(10,ch,horizontal_pixels-strlen(ch)*9,STATUS_AREA_PX+delta+taille,_RED);
	}
      }
    }
    
    // draw
    fltk_draw(*this,g,x_scale,y_scale,clip_x,clip_y,clip_w,clip_h);
    clip_ymin=save_clip_ymin;
    if (hp || tracemode)
      draw_decorations(title_tmp);
  }
  
  void Graph2d::left(double d){ 
    window_xmin -= d;
    window_xmax -= d;
  }

  void Graph2d::right(double d){ 
    window_xmin += d;
    window_xmax += d;
  }

  void Graph2d::up(double d){ 
    window_ymin += d;
    window_ymax += d;
  }

  void Graph2d::down(double d){ 
    window_ymin -= d;
    window_ymax -= d;
  }

  void Graph2d::z_up(double d){ 
    window_zmin += d;
    window_zmax += d;
  }

  void Graph2d::z_down(double d){ 
    window_zmin -= d;
    window_zmax -= d;
  }

  bool global_show_axes=true;
  
  int displaygraph(const giac::gen & ge,const gen & gs,GIAC_CONTEXT){
#ifdef CASCAS_ALEVEL_ONLY
    return -1;
#else
    static gen displayge= ge;
    if (ge!=-1)
      displayge=ge;
    // graph display
    //if (aborttimer > 0) { Timer_Stop(aborttimer); Timer_Deinstall(aborttimer);}
    xcas::Graph2d gr(ge==-1?displayge:ge,contextptr);
    if (gs!=0) gr.symbolic_instructions=gen2vecteur(gs);
    gr.show_axes=global_show_axes;
    gr.init_tracemode();
    if (gr.tracemode & 4)
      gr.orthonormalize(true);
    // initial setting for x and y
    if (ge.type==_VECT){
      const_iterateur it=ge._VECTptr->begin(),itend=ge._VECTptr->end();
      for (;it!=itend;++it){
	if (it->is_symb_of_sommet(at_equal)){
	  const gen & f=it->_SYMBptr->feuille;
	  gen & optname = f._VECTptr->front();
	  gen & optvalue= f._VECTptr->back();
	  if (optname.val==_AXES && optvalue.type==_INT_)
	    gr.show_axes=optvalue.val;
	  if (optname.type==_INT_ && optname.subtype == _INT_PLOT && optname.val>=_GL_X && optname.val<=_GL_Z && optvalue.is_symb_of_sommet(at_interval)){
	    //*logptr(contextptr) << optname << " " << optvalue << endl;
	    gen optvf=evalf_double(optvalue._SYMBptr->feuille,1,contextptr);
	    if (optvf.type==_VECT && optvf._VECTptr->size()==2){
	      gen a=optvf._VECTptr->front();
	      gen b=optvf._VECTptr->back();
	      if (a.type==_DOUBLE_ && b.type==_DOUBLE_){
		switch (optname.val){
		case _GL_X:
		  gr.window_xmin=a._DOUBLE_val;
		  gr.window_xmax=b._DOUBLE_val;
		  gr.update();
		  break;
		case _GL_Y:
		  gr.window_ymin=a._DOUBLE_val;
		  gr.window_ymax=b._DOUBLE_val;
		  gr.update();
		  break;
		case _GL_Z:
		  gr.window_zmin=a._DOUBLE_val;
		  gr.window_zmax=b._DOUBLE_val;
		  gr.update();
		  break;
		}
	      }
	    }
	  }
	}
      }
    }
    return gr.ui();
#endif
  }

  int displaygraph(const giac::gen & ge,const gen & gs){
    return displaygraph(ge,gs,contextptr);
  }

  vecteur Graph2d::get_current_animation() const {
    if (animation_instructions_pos>=0 && animation_instructions_pos<animation_instructions.size())
      return gen2vecteur(animation_instructions[animation_instructions_pos]);
    return 0;
  }

  void Graph2d::find_title_plot(gen & title_tmp,gen & plot_tmp){
    title_tmp=plot_tmp=0;
    if (//in_area &&
	hp && mode && !args_tmp.empty()){
      if (args_tmp.size()>=2){
	gen function=(mode==int(args_tmp.size()))?function_final:function_tmp;
	if (function.type==_FUNC){
	  bool dim2=!is3d;
	  vecteur args2=args_tmp;
	  if ( *function._FUNCptr==(dim2?at_cercle:at_sphere)){
	    gen argv1;
#ifdef NO_STDEXCEPT
	    argv1=evalf(args_tmp.back(),1,contextptr);
	    argv1=evalf_double(argv1,1,contextptr);
#else
	    try {
	      argv1=evalf(args_tmp.back(),1,contextptr);
	      argv1=evalf_double(argv1,1,contextptr);
	    }
	    catch (std::runtime_error & e){
	      argv1=undef;
	    }
#endif
	    if (argv1.is_symb_of_sommet(at_pnt) ||argv1.type==_IDNT){
	      argv1=remove_at_pnt(argv1);
	      if ( (argv1.type==_VECT && argv1.subtype==_POINT__VECT) || argv1.type==_CPLX || argv1.type==_IDNT)
		args2.back()=args_tmp.back()-args_tmp.front();
	    }
	  }
	  if (function==at_ellipse)
	    ;
	  title_tmp=gen(args2,_SEQ__VECT);
	  bool b=approx_mode(contextptr);
	  if (!b)
	    approx_mode(true,contextptr);
	  plot_tmp=symbolic(*function._FUNCptr,title_tmp);
	  if (!lidnt(title_tmp).empty())
	    ; // cerr << plot_tmp << '\n';
	  bool bb=io_graph(contextptr);
	  int locked=0;
	  if (bb){
#ifdef HAVE_LIBPTHREAD
	    // cerr << "plot title lock" << '\n';
	    locked=pthread_mutex_trylock(&interactive_mutex);
#endif
	    if (!locked)
	      io_graph(false,contextptr);
	  }
	  plot_tmp=protecteval(plot_tmp,1,contextptr);
	  if (bb && !locked){
	    io_graph(bb,contextptr);
#ifdef HAVE_LIBPTHREAD
	    pthread_mutex_unlock(&interactive_mutex);
	    // cerr << "plot title unlock" << '\n';
#endif
	  }
	  if (!b)
	    approx_mode(false,contextptr);	
	} // end function.type==_FUNC
	else
	  title_tmp=gen(args_tmp,_SEQ__VECT);
      } // end size()>=2
      else	
	title_tmp=args_tmp;
    }
  }

  void Graph2d::eval(int start){
    plot_instructions.resize(symbolic_instructions.size());
    if (plot_instructions.empty()) return;
    int level=prog_eval_level_val(contextptr);
    for (size_t i=start;i<symbolic_instructions.size();++i){
      gen g=symbolic_instructions[i];
      set_abort();
      g=protecteval(g,level,contextptr);
      clear_abort();
      ctrl_c=false;
      kbd_interrupted=interrupted=false;
      if (i<plot_instructions.size())
	plot_instructions[i]=g;
      else
	plot_instructions.push_back(g);
      if (g.is_symb_of_sommet(at_trace)){
	gen f=symbolic(at_evalf,g._SYMBptr->feuille);
	f=protecteval(f,1,contextptr);
#ifdef NUMWORKS
	const int maxtrace=128;
#else
	const int maxtrace=512;
#endif
	if (trace_instructions.size()>=maxtrace)
	  trace_instructions.erase(trace_instructions.begin(),trace_instructions.begin()+maxtrace/2);
	trace_instructions.push_back(f);
      }
    }
    is3d=false;
    for (size_t i=0;i<plot_instructions.size();++i){
      gen g=plot_instructions[i];
      if (giac::is3d(g)){
	is3d=true;
	update_rotation();
	break;
      }
    }
    update_g();
  }

  double Graph2d::find_eps() const {
    double dx=window_xmax-window_xmin;
    double dy=window_ymax-window_ymin;
    double dz=window_zmax-window_zmin;
    double eps,epsx,epsy;
    int L=LCD_WIDTH_PX;
    epsx=(npixels*dx)/L;
    epsy=(npixels*dy)/(is3d?L:LCD_HEIGHT_PX);
    eps=(epsx<epsy)?epsy:epsx;
    if (is3d && dz>dy && dz >dx){
      eps=npixels*dz/L;
      eps *= 2;
    }
    return eps;
  }

  void Graph2d::set_gen_value(int n,const giac::gen & g,bool exec){
    // set n-th entry value, if n==-1 add a level
    if (!hp) return;
    if (n==-1 || n>=symbolic_instructions.size()){
      symbolic_instructions.push_back(g);
      n=symbolic_instructions.size()-1;
    } else symbolic_instructions[n]=g;
    hp->set_string_value(n,g.print(contextptr));
    if (exec)
      eval(n);
  }

  void  Graph2d::find_xyz(double i,double j,double k,double & x,double & y,double & z) const {
    if (is3d){ // FIXME
      int horiz=width3d/2,vert=horiz/2;//LCD_HEIGHT_PX/2;
      double lcdz= LCD_HEIGHT_PX/4;
      double xmin=-1,ymin=-1,xmax=1,ymax=1,xscale=0.6*(xmax-xmin)/horiz,yscale=0.6*(ymax-ymin)/vert;
      double Z=current_depth; // -1..1
      double I=i-horiz;
      double J=j-LCD_HEIGHT_PX/2+lcdz*Z;
      double X=yscale*J-xscale*I;
      double Y=yscale*J+xscale*I;
      float3d xf,yf,zf;
      do_transform(invtransform,X,Y,Z,xf,yf,zf);
      x=xf; y=yf; z=zf;
    }
    else {
      z=k;
      x=window_xmin+i*(window_xmax-window_xmin)/LCD_WIDTH_PX;
      y=window_ymax-j*(window_ymax-window_ymin)/(LCD_HEIGHT_PX-STATUS_AREA_PX);
    }
  }

  gen geometry_round_numeric(double x,double y,double eps,bool approx){
    return approx?gen(x,y):exact_double(x,eps)+cst_i*exact_double(y,eps);
  }

  gen geometry_round_numeric(double x,double y,double z,double eps,bool approx){
    return gen(approx?makevecteur(x,y,z):makevecteur(exact_double(x,eps),exact_double(y,eps),exact_double(z,eps)),_POINT__VECT);
  }

  gen int2color(int couleur_){
    gen col;
    if (couleur_){
      gen tmp;
      int val;
      vecteur colv;
      if ( (val=(couleur_ & 0x0000ffff))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if ((val =(couleur_ & 0x00070000))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if ((val =(couleur_ & 0x00380000))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if ((val =(couleur_ & 0x01c00000))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if ((val =(couleur_ & 0x0e000000))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if ((val =(couleur_ & 0x30000000))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if ((val =(couleur_ & 0x40000000))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if ((val =(couleur_ & 0x80000000))){
	tmp=val;
	tmp.subtype=_INT_COLOR;
	colv.push_back(tmp);
      }
      if (colv.size()==1)
	col=colv.front();
      else
	col=symbolic(at_plus,gen(colv,_SEQ__VECT));
    }
    return col;
  }

  std::string print_color(int couleur){
    return int2color(couleur).print(context0);
  }

  giac::gen add_attributs(const giac::gen & g,int couleur_,GIAC_CONTEXT) {
    if (g.type!=_SYMB)
      return g;
    gen & f=g._SYMBptr->feuille;
    if (g._SYMBptr->sommet==at_couleur && f.type==_VECT && !f._VECTptr->empty()){
      gen col=couleur_;
      col.subtype=_INT_COLOR;
      vecteur v(*f._VECTptr);
      v.back()=col;
      return symbolic(at_couleur,gen(v,_SEQ__VECT));
    }
    if (couleur_==default_color(contextptr))
      return g;
    if (g._SYMBptr->sommet==at_of){
      gen col=couleur_;
      col.subtype=_INT_COLOR;
      return symbolic(at_couleur,gen(makevecteur(g,col),_SEQ__VECT));
    }
    vecteur v =gen2vecteur(f);
    gen col=int2color(couleur_);
    v.push_back(symbolic(at_equal,gen(makevecteur(at_display,col),_SEQ__VECT)));
    return symbolic(g._SYMBptr->sommet,(v.size()==1 && f.type!=_VECT)?f:gen(v,f.type==_VECT?f.subtype:_SEQ__VECT));
  }

  void Graph2d::do_handle(const gen & g){
    if (hp){
      set_gen_value(hp_pos,g,true);
    }
  }

  std::string printn(const gen & g,int n){
    if (g.type!=_DOUBLE_)
      return g.print();
    return giac::print_DOUBLE_(g._DOUBLE_val,n);
  }
  gen * x0ptr(){
    static gen * ptr=0;
    if (!ptr){
      ptr=new gen;
      *ptr=gen("x0",contextptr);
    }
    return ptr;
  }
  gen * x1ptr(){
    static gen * ptr=0;
    if (!ptr){
      ptr=new gen;
      *ptr=gen("x1",contextptr);
    }
    return ptr;
  }
  gen * x2ptr(){
    static gen * ptr=0;
    if (!ptr){
      ptr=new gen;
      *ptr=gen("x2",contextptr);
    }
    return ptr;
  }
  gen * y0ptr(){
    static gen * ptr=0;
    if (!ptr){
      ptr=new gen;
      *ptr=gen("y0",contextptr);
    }
    return ptr;
  }
  gen * y1ptr(){
    static gen * ptr=0;
    if (!ptr){
      ptr=new gen;
      *ptr=gen("y1",contextptr);
    }
    return ptr;
  }
  gen * y2ptr(){
    static gen * ptr=0;
    if (!ptr){
      ptr=new gen;
      *ptr=gen("y2",contextptr);
    }
    return ptr;
  }
  void Graph2d::tracemode_set(int operation){
    if (plot_instructions.empty())
      plot_instructions=gen2vecteur(g);
    if (is_zero(plot_instructions.back())) // workaround for 0 at end in geometry (?)
      plot_instructions.pop_back();
    gen sol(undef);
    if (operation==1 || operation==8){
      double d=tracemode_mark;
      if (!inputdouble(lang==1?"Valeur du parametre?":"Parameter value",d,contextptr))
	return;
      if (operation==8)
	tracemode_mark=d;
      sol=d;
    }
    // handle curves with more than one connected component
    vecteur tracemode_v;
    for (int i=0;i<plot_instructions.size();++i){
      gen g=plot_instructions[i];
      if (g.type==_VECT && !g._VECTptr->empty() && g._VECTptr->front().is_symb_of_sommet(at_curve)){
	vecteur & v=*g._VECTptr;
	for (int j=0;j<v.size();++j)
	  tracemode_v.push_back(v[j]);
      }
      else
	tracemode_v.push_back(g);
    }
    gen G;
    if (tracemode_n<0)
      tracemode_n=tracemode_v.size()-1;
    bool retry=tracemode_n>0;
    for (;tracemode_n<tracemode_v.size();++tracemode_n){
      G=tracemode_v[tracemode_n];
      if (G.is_symb_of_sommet(at_pnt))
	break;
    }
    if (tracemode_n>=tracemode_v.size()){
      // retry
      if (retry){
	for (tracemode_n=0;tracemode_n<tracemode_v.size();++tracemode_n){
	  G=tracemode_v[tracemode_n];
	  if (G.is_symb_of_sommet(at_pnt))
	    break;
	}
      }
      if (tracemode_n>=tracemode_v.size()){
	tracemode=false;
	return;
      }
    }
    int p=python_compat(contextptr);
    python_compat(0,contextptr);
    gen G_orig(G);
    G=remove_at_pnt(G);
    tracemode_disp.clear();
    string curve_infos1,curve_infos2;
    gen parameq,x,y,t,tmin,tmax,tstep,x1,x2,y1,y2,z1,z2;
    // extract position at tracemode_i
    if (G.is_symb_of_sommet(at_curve)){
      gen c=G._SYMBptr->feuille[0];
      parameq=c[0];
      t=c[1];
      if (parameq==trace_parameq && t==trace_t){
        x=trace_x0;
        x1=trace_x1;
        x2=trace_x2;
        y=trace_y0;
        y1=trace_y1;
        y2=trace_y2;
        z1=trace_z1;
        z2=trace_z2;
      }
      else {
        trace_t=t;
        trace_parameq=parameq;
        z1=trace_z1=derive(parameq,t,contextptr);
        z2=trace_z2=derive(z1,t,contextptr);
        // simple expand for i*ln(x)
        bool b=do_lnabs(contextptr);
        do_lnabs(false,contextptr);
        reim(parameq,x,y,contextptr);
        do_lnabs(b,contextptr);
        trace_x0=x;
        trace_x1=x1=derive(x,t,contextptr);
        trace_x2=x2=derive(x1,t,contextptr);
        trace_y0=y;
        trace_y1=y1=derive(y,t,contextptr);
        trace_y2=y2=derive(y1,t,contextptr);
        sto(x,*x0ptr(),contextptr);
        sto(x1,*x1ptr(),contextptr);
        sto(x2,*x2ptr(),contextptr);
        sto(y,*y0ptr(),contextptr);
        sto(y1,*y1ptr(),contextptr);
        sto(y2,*y2ptr(),contextptr);
      }
      tmin=c[2];
      tmax=c[3];
      tmin=evalf_double(tmin,1,contextptr);
      tmax=evalf_double(tmax,1,contextptr);
      if (tmin._DOUBLE_val>tracemode_mark)
	tracemode_mark=tmin._DOUBLE_val;
      if (tmax._DOUBLE_val<tracemode_mark)
	tracemode_mark=tmax._DOUBLE_val;
      G=G._SYMBptr->feuille[1];
      if (G.type==_VECT && !G._VECTptr->empty()){
	vecteur &Gv=*G._VECTptr;
        bool doit=false;
        if (x==t){
          tmin=re(Gv.front(),contextptr);
          tmax=re(Gv.back(),contextptr);
          doit=true;
        }
        else if (y==t){
          tmin=im(Gv.front(),contextptr);
          tmax=im(Gv.back(),contextptr);
          doit=true;
        } else
          tstep=(tmax-tmin)/(Gv.size()-1); // might be wrong...          
        if (doit){
          tstep=(tmax-tmin)/(Gv.size()-1);
          if (tracemode_mark<tmin._DOUBLE_val)
            tracemode_mark=tmin._DOUBLE_val;
          if (tracemode_mark>tmax._DOUBLE_val)
            tracemode_mark=tmax._DOUBLE_val;
        }
      }
      double eps=1e-6; // epsilon(contextptr)
      double curt=(tmin+tracemode_i*tstep)._DOUBLE_val;
      if (abs(curt-tracemode_mark)<.999*tstep._DOUBLE_val)
	curt=tracemode_mark;
      if (operation==-1){
	gen A,B,C,R; // detect ellipse/hyperbola
	if (
	    ( x!=t && c.type==_VECT && c._VECTptr->size()>7 && centre_rayon(G_orig,C,R,false,contextptr,true) ) ||
	    is_quadratic_wrt(parameq,t,A,B,C,contextptr)
	    ){
	  if (C.type!=_VECT){ // x+i*y=A*t^2+B*t+C
	    curve_infos1="Parabola";
	    curve_infos2=_equation(G_orig,contextptr).print(contextptr);
	  }
	  else {
	    vecteur V(*C._VECTptr);
	    curve_infos1=V[0].print(contextptr);
	    curve_infos1=curve_infos1.substr(1,curve_infos1.size()-2);
	    curve_infos1+=" O=";
	    curve_infos1+=V[1].print(contextptr);
	    curve_infos1+=", F=";
	    curve_infos1+=V[2].print(contextptr);
	    // curve_infos1=change_subtype(C,_SEQ__VECT).print(contextptr);
	    curve_infos2=change_subtype(R,_SEQ__VECT).print(contextptr);
	  }
	}
	else {
	  if (x==t) curve_infos1="Function "+y.print(contextptr); else curve_infos1="Parametric "+x.print(contextptr)+","+y.print(contextptr);
	  curve_infos2 = t.print(contextptr)+"="+tmin.print(contextptr)+".."+tmax.print(contextptr)+',';
	  curve_infos2 += (x==t?"xstep=":"tstep=")+tstep.print(contextptr);
	}
      }
      if (operation==1)
	curt=sol._DOUBLE_val;
      if (operation==7)
	sol=tracemode_mark=curt;
      if (operation==2){ // root near curt
	sol=newton(y,t,curt,NEWTON_DEFAULT_ITERATION,eps,1e-12,true,tmin._DOUBLE_val,tmax._DOUBLE_val,tmin._DOUBLE_val,tmax._DOUBLE_val,1,contextptr);
	if (sol.type==_DOUBLE_){
	  confirm(lang==1?"Racine en":"Root at",sol.print(contextptr).c_str());
	  sto(sol,gen("Zero",contextptr),contextptr);
	}
      }
      if (operation==4){ // horizontal tangent near curt
	sol=newton(y1,t,curt,NEWTON_DEFAULT_ITERATION,eps,1e-12,true,tmin._DOUBLE_val,tmax._DOUBLE_val,tmin._DOUBLE_val,tmax._DOUBLE_val,1,contextptr);
	if (sol.type==_DOUBLE_){
	  confirm(lang==1?"y'=0, extremum/pt singulier en":"y'=0, extremum/singular pt at",sol.print(contextptr).c_str());
	  sto(sol,gen("Extremum",contextptr),contextptr);
	}
      }
      if (operation==5){ // vertical tangent near curt
	if (x1==1)
	  do_confirm(lang==1?"Outil pour courbes parametriques!":"Tool for parametric curves!");
	else {
	  sol=newton(x1,t,curt,NEWTON_DEFAULT_ITERATION,eps,1e-12,true,tmin._DOUBLE_val,tmax._DOUBLE_val,tmin._DOUBLE_val,tmax._DOUBLE_val,1,contextptr);
	  if (sol.type==_DOUBLE_){
	    confirm("x'=0, vertical or singular",sol.print(contextptr).c_str());
	    sto(sol,gen("Vertical",contextptr),contextptr);
	  }
	}
      }
      if (operation==6){ // inflexion
	sol=newton(x1*y2-x2*y1,t,curt,NEWTON_DEFAULT_ITERATION,eps,1e-12,true,tmin._DOUBLE_val,tmax._DOUBLE_val,tmin._DOUBLE_val,tmax._DOUBLE_val,1,contextptr);
	if (sol.type==_DOUBLE_){
	  confirm("x'*y''-x''*y'=0",sol.print(contextptr).c_str());
	  sto(sol,gen("Inflexion",contextptr),contextptr);
	}
      }
      gen M(put_attributs(_point(subst(parameq,t,tracemode_mark,false,contextptr),contextptr),vecteur(1,_POINT_WIDTH_4 | _BLUE),contextptr));
      tracemode_disp.push_back(M);      
      gen f;
      if (operation==9)
	f=y*derive(x,t,contextptr);
      if (operation==10){
	f=sqrt(pow(x1,2,contextptr)+pow(y1,2,contextptr),contextptr);
      }
      if (operation==9 || operation==10){
	double a=tracemode_mark,b=curt;
	if (a>b)
	  swapdouble(a,b);
	gen res=symbolic((operation==9 && x==t?at_plotarea:at_integrate),
			  makesequence(f,symb_equal(t,symb_interval(a,b))));
	if (operation==9)
	  tracemode_disp.push_back(giac::eval(res,1,contextptr));
	string ss=res.print(contextptr);
	if (!tegral(f,t,a,b,1e-6,1<<10,res,false,contextptr))
	  confirm("Numerical Integration Error",ss.c_str());
	else {
	  confirm(ss.c_str(),res.print(contextptr).c_str());
	  sto(res,gen((operation==9?"Area":"Arclength"),contextptr),contextptr);	  
	}
      }
      if (operation>=1 && operation<=8 && sol.type==_DOUBLE_ && !is_zero(tstep)){
	tracemode_i=(sol._DOUBLE_val-tmin._DOUBLE_val)/tstep._DOUBLE_val;
	G=subst(parameq,t,sol._DOUBLE_val,false,contextptr);
      }
    }
    if (G.is_symb_of_sommet(at_cercle)){
      if (operation==-1){
	gen c,r;
	centre_rayon(G,c,r,true,contextptr);
	curve_infos1="Circle radius "+r.print(contextptr);
	curve_infos2="Center "+_coordonnees(c,contextptr).print(contextptr);
      }
      G=G._SYMBptr->feuille[0];
    }
    bool implicit=false;
    if (G.type==_VECT){
      vecteur & v=*G._VECTptr;
      if (v.size()>16){
        gen x2(x2ptr()->eval(1,contextptr));
        implicit=x2.type==_STRNG && *x2._STRNGptr=="implicit";
      }
      if (operation==-1 && curve_infos1.size()==0){
	if (v.size()==2)
	  curve_infos1=_equation(G_orig,contextptr).print(contextptr);
	else if (v.size()==4)
	  curve_infos1="Triangle";
	else curve_infos1="Polygon";
	curve_infos2=G.print(contextptr);
      }
      int i=std::floor(tracemode_i);
      double id=tracemode_i-i;
      if (i>=int(v.size()-1)){
	tracemode_i=i=v.size()-1;
	id=0;
      }
      if (i<0){
	tracemode_i=i=0;
	id=0;
      }
      G=v[i];
      if (!is_zero(tstep) && id>0)
	G=v[i]+id*tstep*(v[i+1]-v[i]);
    }
    G=evalf(G,1,contextptr);
    if (operation==3){ // intersect this curve with all other curves
      vecteur V;
      for (int j=0;j<tracemode_v.size();++j){
	if (j==tracemode_n)
	  continue;
	gen H=tracemode_v[j];
	gen I=_inter(makesequence(G_orig,H),contextptr);
	if (I.type==_VECT)
	  V=mergevecteur(V,*I._VECTptr);
      }
      sto(V,gen("Intersect",contextptr),contextptr);
      tracemode_disp.clear();
      tracemode_disp.push_back(put_attributs(V,vecteur(1,_POINT_WIDTH_6 | _RED),contextptr));
      if (!V.empty()){
	gen I1(undef),I2(undef),d1(plus_inf),d2(plus_inf);
	for (int i=0;i<V.size();++i){
	  gen cur=evalf_double(V[i],1,contextptr);
	  if (i==0){
	    I1=cur; d1=distance2pp(I1,G,contextptr);
	    continue;
	  }
	  if (i==1){
	    I2=cur; d2=distance2pp(I2,G,contextptr);
	    if (is_strictly_greater(d1,d2,contextptr)){
	      swapgen(I1,I2); swapgen(d1,d2);
	    }
	    continue;
	  }
	  gen d=distance2pp(cur,G,contextptr);
	  if (is_strictly_greater(d1,d,contextptr)){
	    I2=I1; d2=d1;
	    I1=cur; d1=d;
	    continue;
	  }
	  if (is_strictly_greater(d2,d,contextptr)){
	    I2=cur; d2=d;
	  }
	} // end for loop in V
	G=remove_at_pnt(I2);
	I1=put_attributs(I1,vecteur(1,_POINT_WIDTH_6 | _BLUE),contextptr);
	tracemode_disp.push_back(I1);      
	if (is_undef(I2)) I2=I1;
	I2=put_attributs(I2,vecteur(1,_POINT_WIDTH_6 | _BLUE),contextptr);
	tracemode_disp.push_back(I2);      
	// function curve: set nearest intersection as mark/position
	if (t==x && !is_zero(tstep)){
	  gen Ix,Iy;
	  reim(remove_at_pnt(I1),Ix,Iy,contextptr);
	  tracemode_mark=evalf_double(Ix,1,contextptr)._DOUBLE_val;
	  reim(remove_at_pnt(I2),Ix,Iy,contextptr);
	  tracemode_i=((evalf_double(Ix,1,contextptr)-tmin)/tstep)._DOUBLE_val;
	}
      }
    } // end intersect
    gen Gx,Gy; reim(G,Gx,Gy,contextptr);
    Gx=evalf_double(Gx,1,contextptr);
    Gy=evalf_double(Gy,1,contextptr);
    if (operation==-1){
      if (curve_infos1.size()==0)
	curve_infos1="Position "+Gx.print(contextptr)+","+Gy.print(contextptr);
      if (G_orig.is_symb_of_sommet(at_pnt)){
	gen f=G_orig._SYMBptr->feuille;
	if (f.type==_VECT && f._VECTptr->size()==3){
	  f=f._VECTptr->back();
	  curve_infos1 = f.print(contextptr)+": "+curve_infos1;
	}
      }
      // confirm(curve_infos1.c_str(),curve_infos2.c_str());
      if (confirm(curve_infos1.c_str(),curve_infos2.c_str())==KEY_CTRL_F1 && tstep!=0){
	double t0=tmin._DOUBLE_val,ts,tc=t0;
	ts=find_tick(tstep._DOUBLE_val*5);
	t0=int(t0/ts)*ts;
	int ndisp=10,N=6,dy=24;
	for (;;){
#ifdef NUMWORKS
	  statuslinemsg("Back: quit, up/down: move");
#else
	  statuslinemsg("esc: quit, up/down: move");
#endif
	  // table of values
	  drawRectangle(0,dy,LCD_WIDTH_PX,LCD_HEIGHT_PX-dy,_WHITE);
	  if (t==x){
	    os_draw_string(0,dy,_BLACK,_WHITE,"x");
	    os_draw_string(120,dy,_BLACK,_WHITE,y.print().c_str());
	  }
	  else {
	    os_draw_string(0,dy,_BLACK,_WHITE,"t");
	    os_draw_string(107,dy,_BLACK,_WHITE,"x");
	    os_draw_string(214,dy,_BLACK,_WHITE,"y");
	  }
	  vecteur V;
	  for (int i=1;i<=ndisp;++i){
	    double tcur=tc+(i-1)*ts;
	    vecteur L(1,tcur);
	    os_draw_string(0,dy+i*18,_BLACK,_WHITE,printn(tcur,N).c_str());
	    if (t==x){
	      gen cur=subst(y,t,tcur,false,contextptr);
	      L.push_back(cur);
	      os_draw_string(120,dy+i*18,_BLACK,_WHITE,printn(cur,N).c_str());
	    }
	    else {
	      gen cur=subst(x,t,tcur,false,contextptr);
	      L.push_back(cur);
	      os_draw_string(107,dy+i*18,_BLACK,_WHITE,printn(cur,N).c_str());
	      cur=subst(y,t,tcur,false,contextptr);
	      L.push_back(cur);
	      os_draw_string(214,dy+i*18,_BLACK,_WHITE,printn(cur,N).c_str());	      
	    }
	    V.push_back(L);
	  }
	  int key; ck_getkey(&key);
	  if (key==KEY_CTRL_EXIT || key==KEY_CTRL_OK)
	    break;
	  if (key==KEY_CTRL_UP)
	    tc -= (ndisp/2)*ts;
	  if (key==KEY_CTRL_DOWN)
	    tc += (ndisp/2)*ts;
	  if (key=='+')
	    ts /= 2;
	  if (key=='-')
	    ts *= 2;
	  if (key==KEY_CTRL_DEL && inputdouble("step",ts,contextptr))
	    ts=fabs(ts);
	  if (key==KEY_CTRL_LEFT)
	    inputdouble("min",tc,contextptr);
	  if (key==KEY_CTRL_CLIP)
	    copy_clipboard(gen(V).print(contextptr),true,true);
	}
      }
    }
    tracemode_add="";
    if (Gx.type==_DOUBLE_ && Gy.type==_DOUBLE_){
      tracemode_add += "x=";
      tracemode_add += print_DOUBLE_(Gx._DOUBLE_val,3);
      tracemode_add += ",y=";
      tracemode_add += print_DOUBLE_(Gy._DOUBLE_val,3);
      if (implicit){
        sto(Gx,*x0ptr(),contextptr);
        sto(Gy,*y0ptr(),contextptr);
        gen m(gen(makevecteur(*x1ptr(),*y1ptr())).eval(1,contextptr));
        m=subst(m,makevecteur(x__IDNT_e,y__IDNT_e),makevecteur(Gx,Gy),false,contextptr);
        m=evalf_double(m,1,contextptr);
        m=_normalize(m,contextptr);
        gen T(_droite(makesequence(_point(G,contextptr),_point(G+m[0]+cst_i*m[1],contextptr)),contextptr));
        m=m[1]/m[0];
        if (m.type==_DOUBLE_){
          tracemode_add += " ,m=";
          tracemode_add += giac::print_DOUBLE_(m._DOUBLE_val,3);
          tracemode_disp.push_back(T);
        }
      }
      if (tstep!=0){
	gen curt=tmin+tracemode_i*tstep;
	if (curt.type==_DOUBLE_){
	  if (t!=x)
	    tracemode_add += ", t="+print_DOUBLE_(curt._DOUBLE_val,3);
	  if (tracemode & 2){
            // make sure G is the right point, e.g. for plotpolar(sqrt(cos(2x)))
            G=subst(parameq,t,curt,false,contextptr);
	    //gen G1=derive(parameq,t,contextptr);
	    gen G1t=subst(z1,t,curt,false,contextptr);
	    gen G1x,G1y; reim(G1t,G1x,G1y,contextptr);
            // gen G1x=subst(x1,t,curt,false,contextptr),G1y=subst(y1,t,curt,false,contextptr);
	    gen m=evalf_double(G1y/G1x,1,contextptr);
	    if (m.type==_DOUBLE_)
	      tracemode_add += ", m="+giac::print_DOUBLE_(m._DOUBLE_val,3);
	    gen T(_vector(makesequence(_point(G,contextptr),_point(G+G1t,contextptr),symb_equal(at_display,_BLUE)),contextptr));
	    tracemode_disp.push_back(T);
            if (tracemode & 0xc){
              //gen G2(derive(G1,t,contextptr));
              gen G2t=subst(z2,t,curt,false,contextptr);
              gen G2x,G2y; reim(G2t,G2x,G2y,contextptr);
              // gen G2x=subst(x2,t,curt,false,contextptr),G2y=subst(y2,t,curt,false,contextptr);
              //dbg_printf("tracemode G2x=%s G2y=%s\n",G2x.print().c_str(),G2y.print().c_str());
              //gen det(re(G1t*conj(G2t,contextptr),contextptr)); 
              gen det(G1x*G2y-G2x*G1y);
              gen Tn=abs(G1t,contextptr);
              gen R=evalf_double(Tn*Tn*Tn/det,1,contextptr);
              gen centre=G+R*cst_i*G1t/Tn;
              // gen centre=G+R*(-G1y+cst_i*G1x)/Tn;
              if (tracemode & 4){
                gen N(_vector(makesequence(_point(G,contextptr),_point(centre,contextptr),symb_equal(at_display,_BLUE)),contextptr));
                tracemode_disp.push_back(N);
              }
              if (tracemode & 8){
                if (R.type==_DOUBLE_)
                  tracemode_add += ", R="+giac::print_DOUBLE_(R._DOUBLE_val,3);
                tracemode_disp.push_back(_cercle(makesequence(centre,R,symb_equal(at_display,_BLUE)),contextptr));
              }
            }
          }
	}
      }
    }
    double x_scale=LCD_WIDTH_PX/(window_xmax-window_xmin);
    double y_scale=(LCD_HEIGHT_PX-STATUS_AREA_PX)/(window_ymax-window_ymin);
    double i,j;
    findij(G,x_scale,y_scale,i,j,contextptr);
    current_i=int(i+.5);
    current_j=int(j+.5);
    python_compat(p,contextptr);
  }

  void Graph2d::invert_tracemode(){
    if (!tracemode)
      init_tracemode();
    else
      tracemode=0;
  }

  void Graph2d::init_tracemode(){
    if (is3d){
      tracemode=0;
      return;
    }
    tracemode_mark=0.0;
    double w=LCD_WIDTH_PX;
    double h=LCD_HEIGHT_PX-STATUS_AREA_PX;
    double window_w=window_xmax-window_xmin,window_h=window_ymax-window_ymin;
    double r=h/w*window_w/window_h;
    tracemode=(r>0.7 && r<1.4)?7:3;
    tracemode_set();
  }

  void Graph2d::curve_infos(){
    if (!tracemode)
      init_tracemode();
    const char *
      tab[]={
	     lang==1?"Infos objet (F2)":"Object infos (F2)",  // 0
	     lang==1?"Quitte mode etude (xtt)":"Quit study mode (xtt)",
	     lang==1?"Entrer t ou x":"Set t or x", // 1
	     lang==1?"y=0, racine":"y=0, root",
	     "Intersection", // 3
	     "y'=0, extremum",
	     lang==1?"x'=0 (parametriques)":"x'=0 (parametric)", // 5
	     "Inflexion",
	     lang==1?"Marquer la position":"Mark position",
	     lang==1?"Entrer t ou x, marquer":"Set t or x, mark", // 8
	     lang==1?"Aire":"Area",
	     lang==1?"Longueur d'arc":"Arc length", // 10
	     0};
    const int s=sizeof(tab)/sizeof(char *);
    int choix=select_item(tab,lang==1?"Etude courbes":"Curve study",true);
    if (choix<0 || choix>s)
      return;
    if (choix==1)
      tracemode=0;
    else 
      tracemode_set(choix-1);
  }

  void Graph2d::set_mode(const giac::gen & f_tmp,const giac::gen & f_final,int m,const string & help){
    approx=true;
    mode=m;
    selected.clear();
    redraw();
    args_help.clear();
    if (mode!=0 && mode!=255){
      int oldmode=calc_mode(contextptr);
      calc_mode(0,contextptr);
      gen g(help,contextptr);
      calc_mode(oldmode,contextptr);
      if (g.type==_VECT){
	const_iterateur it = g._VECTptr->begin(),itend=g._VECTptr->end();
	for (;it!=itend;++it)
	  args_help.push_back(it->print(contextptr));
      }
      else
	args_help.push_back(g.print(contextptr));
    }
    if (mode==255)
      modestr=gettext("Frame");
    else
      modestr=mode?gen2string(f_final).c_str():gettext("Pointer");
    if (mode>=-1){
      pushed=false;
      moving_param=moving=moving_frame=false;
      // history_pos=-1;
      mode=m;
      function_final=f_final;
      function_tmp=f_tmp;
      args_tmp.clear();
      geo_handle(FL_MOVE,0);
      update_g();
    }
  }

  vecteur Graph2d::selection2vecteur(const vector<int> & v){
    int n=v.size();
    vecteur res(n);
    for (int i=0;i<n;++i){
      res[i]=plot_instructions[v[i]];
    }
    return res;
  }

  int findfirstclosedcurve(const vecteur & v){
    int s=v.size();
    for (int i=0;i<s;++i){
      gen g=remove_at_pnt(v[i]);
      if (g.is_symb_of_sommet(at_cercle))
	return i;
      if (g.type==_VECT && g.subtype==_GROUP__VECT){
	vecteur & w=*g._VECTptr;
	if (!w.empty() && w.front()==w.back())
	  return i;
      }
    }
    return -1;
  }

  void Graph2d::geometry_round(double x,double y,double z,double eps,gen & tmp,GIAC_CONTEXT)  {
    tmp=is3d?geometry_round_numeric(x,y,z,eps,approx):geometry_round_numeric(x,y,eps,approx);
    selected=nearest_point(plot_instructions,is3d?geometry_round_numeric(x,y,z,eps,true):geometry_round_numeric(x,y,eps,true),eps,contextptr);
    // bug bonux: when a figure is saved, plot_instructions is saved
    // if there are sequences in plot_instructions
    // they are not put back in an individual level
    while (!selected.empty() && selected.back()>=hp->elements.size())
      selected.pop_back();
  }

  gen Graph2d::geometry_round(double x,double y,double z,double eps,gen & original,int & pos,bool selectfirstlevel,bool setscroller) {
    if (!hp)
      return undef;
    gen tmp;
    pos=-1;
    geometry_round(x,y,z,eps,tmp,contextptr);
    if (selected.empty())
      return tmp;
    if (function_final==at_areaatraw || function_final==at_areaat || function_final==at_perimeteratraw || function_final==at_perimeterat){
      int p=findfirstclosedcurve(selection2vecteur(selected));
      if (p>0){
	pos=p;
      }
    }
    if (pos==-1){
      if (selectfirstlevel){
	sort(selected.begin(),selected.end());
	// patch so that we move element and not the curve
	int p=findfirstpoint(selection2vecteur(selected));
	if (p>0){
	  pos=p;
	}
      }
      else
	pos=findfirstpoint(selection2vecteur(selected));
    }
    gen g=symbolic_instructions[ (pos<0)?(pos=selected.front()):(pos=selected[pos]) ];
    if (pos>=0 && pos<hp->elements.size()){
      // hp->_sel_begin=hp->_sel_end=pos;
      // if (setscroller) hp->line=pos;
    }
    if (g.is_symb_of_sommet(at_plus) && g._SYMBptr->feuille.type==_VECT && !g._SYMBptr->feuille._VECTptr->empty())
      g=g._SYMBptr->feuille._VECTptr->front();
    if (g.is_symb_of_sommet(at_sto) && g._SYMBptr->feuille.type==_VECT ){
      vecteur & v = *g._SYMBptr->feuille._VECTptr;
      if (v.size()==2){
	original = v[0];
	tmp = v[1];
	if (tmp.type==_IDNT){
	  gen valeur=protecteval(original,1,contextptr);
	  if (valeur.is_symb_of_sommet(at_pnt)){
	    gen & valf = valeur._SYMBptr->feuille;
	    if (valf.type==_VECT){
	      vecteur & valv = *valf._VECTptr;
	      int s=v.size();
	      if (s>1){
		gen valv1=valv[1];
		if (valv1.type==_VECT && valv1._VECTptr->size()>2){
		  tmp=symbolic(at_extract_measure,v[1]);
		}
	      }
	    }
	  }
	}
      }
    }
    return tmp;
  }

  void Graph2d::autoname_plus_plus(){
    if (hp){
      string s=autoname(contextptr);
      giac::autoname_plus_plus(s);
      autoname(s,contextptr);
    }
  }
  
  int Graph2d::geo_handle(int event,int key){
    double eps=find_eps();
    int pos;
    gen tmp,tmp2,decal;
    if (event==FL_PUSH)
      moving_param=false;
    if ( pushed && !moving && !moving_frame && mode ==0 && in_area && event==FL_DRAG){
      // FIXME? redraw();
      return 1;
    }
    if (mode>=2 && event==FL_MOVE && args_tmp.size()>mode)
      event=FL_RELEASE;
    if ( in_area && ((mode!=1 && event==FL_DRAG) || event==FL_PUSH || event==FL_RELEASE || (mode>=2 && event==FL_MOVE)) ){
      double newx,newy,newz;
      find_xyz(current_i,current_j,current_depth,newx,newy,newz);
      round3(newx,window_xmin,window_xmax);
      round3(newy,window_ymin,window_ymax);
      if (is3d)
	round3(newz,window_zmin,window_zmax);
      tmp=geometry_round(newx,newy,newz,eps,tmp2,pos,mode==0 || (args_tmp.size()==mode && function_final.type==_FUNC && equalposcomp(transformation_functions,*function_final._FUNCptr)),event==FL_RELEASE);
      if (tmp.type!=_IDNT && !tmp.is_symb_of_sommet(at_extract_measure)){
	bool done=false;
	if (mode==0 && event==FL_PUSH && current_i<192 && current_j<14*nparams+21){
	  double d=current_j/14.-2;
	  //confirm(print_INT_(current_j).c_str(),"");
	  vecteur vp=param(d);
	  if (vp.size()==2){
	    tmp=vp[0][0];
	    tmp2=vp[0];
	    pos=vp[1].val;
	    done=moving_param=true;
	    param_min=evalf_double(tmp2[1],1,contextptr)._DOUBLE_val;
	    param_max=evalf_double(tmp2[2],1,contextptr)._DOUBLE_val;
	    param_step=evalf_double(tmp2[4],1,contextptr)._DOUBLE_val;
	    param_orig=param_value=evalf_double(tmp2[3],1,contextptr)._DOUBLE_val;
	  }
	}
	if (!done){
	  if (tmp.type==_VECT && tmp._VECTptr->size()==3){
	    tmp.subtype=_SEQ__VECT;
	    tmp=symbolic(at_point,tmp);
	  }
	  else
	    tmp=symbolic(at_point,makevecteur(re(tmp,contextptr),im(tmp,contextptr)));
	}
      }
    }
    double newx,newy,newz;
    if (is3d){
      double x1,y1,z1,x2,y2,z2;
      find_xyz(current_i,current_j,current_depth,x1,y1,z1);
      find_xyz(push_i,push_j,push_depth,x2,y2,z2);
      newx=x1-x2; newy=y1-y2; newz=z1-z2;
    } else {
      int dw=LCD_WIDTH_PX,dh=LCD_HEIGHT_PX-STATUS_AREA_PX;
      double dx=window_xmax-window_xmin;
      double dy=window_ymax-window_ymin;
      double x_scale=dx/dw,y_scale=dy/dh;
      newx=(current_i-push_i)*x_scale;
      newy=(push_j-current_j)*y_scale;
      newz=0;
    }
    round3(newx,window_xmin,window_xmax);
    round3(newy,window_ymin,window_ymax);      
    if (is3d){
      round3(newz,window_zmin,window_zmax);
      decal=in_area?geometry_round_numeric(newx,newy,newz,eps,approx):0;
      if (decal.type==_VECT && decal.subtype==_POINT__VECT)
	decal.subtype=0;
    }
    else
      decal=in_area?geometry_round_numeric(newx,newy,eps,approx):0;
    // cerr << in_area << " " << decal << '\n';
    if (mode==0 || mode==255) {
      if (event==FL_PUSH){ 
	// select object && flag to move it 
	if (mode==0 && pos>=0){
	  if (tmp.type==_IDNT){
	    drag_original_value=tmp2;
	    drag_name=tmp;
	  }
	  else {
	    drag_original_value=symbolic_instructions[pos];
	    drag_name=0;
	  }
	  hp_pos=pos;
	  moving = true;
	}
	else { // nothing selected, move frame
	  if (!(display_mode & 0x80)) // disabled by default in 3-d 
	    moving_frame=true;
	}
	return 1;
      }
      if (moving_frame && (event==FL_DRAG || event==FL_RELEASE) ){
	window_xmin -= newx;
	window_xmax -= newx;
	window_ymin -= newy;
	window_ymax -= newy;
	window_zmin -= newz;
	window_zmax -= newz;
	push_i = current_i;
	push_j = current_j;
	push_depth = current_depth;
	redraw();
	if (event==FL_RELEASE)
	  moving_frame=false;
	return 1;
      }
      if (mode==255)
	return 0;
      if (moving_param && (event==FL_DRAG || event==FL_RELEASE) ){
	// key ->
	if (key==KEY_CTRL_EXIT)
	  param_value=param_orig;
	double ps=param_step;
	if (ps<=0)
	  ps=(param_max-param_min)/100;
	if (key==KEY_CTRL_LEFT)
	  param_value -= ps;
	if (key==KEY_SHIFT_LEFT)
	  param_value -= 10*ps;
	if (key==KEY_CTRL_RIGHT)
	  param_value += ps;
	if (key==KEY_SHIFT_RIGHT)
	  param_value += 10*ps;
	if (param_value<param_min)
	  param_value=param_min;
	if (param_value>param_max)
	  param_value=param_max;
	current_i=64+128*(param_value-param_min)/(param_max-param_min);
	if (param_step<=0)
	  do_handle(symbolic(at_assume,symb_equal(drag_name,param_value)));
	else {
	  gen newval=symbolic(at_element,makesequence(symb_interval(param_min,param_max),param_value));
	  do_handle(symbolic(at_sto,makevecteur(newval,drag_name)));
	}
	if (event==FL_RELEASE){
	  moving_param=moving=false;
	}
	return 1;
      }
      if (moving && (event==FL_DRAG || event==FL_RELEASE) ){
	// cerr << current_i << " " << current_j << '\n';
	// avoid point()+complex+complex+complex
	gen newval=drag_original_value;
	if (in_area && key!=KEY_CTRL_EXIT){
	  if (drag_original_value.is_symb_of_sommet(at_plus) && drag_original_value._SYMBptr->feuille.type==_VECT && drag_original_value._SYMBptr->feuille._VECTptr->size()>=2){
	    vecteur v=*drag_original_value._SYMBptr->feuille._VECTptr;
	    if (v[1].is_symb_of_sommet(at_nop))
	      v[1]=v[1]._SYMBptr->feuille;
	    newval=symbolic(at_plus,makevecteur(v[0],symbolic(at_nop,ratnormal(_plus(vecteur(v.begin()+1,v.end()),contextptr)+decal))));
	  }
	  else {
	    newval=is_zero(decal)?drag_original_value:symbolic(at_plus,makevecteur(drag_original_value,symbolic(at_nop,decal)));
	  }
	}
	int dclick = 0 || drag_original_value.type==_VECT;
	if (!dclick){
	  if (drag_name.type==_IDNT)
	    do_handle(symbolic(at_sto,makevecteur(newval,drag_name)));
	  else
	    do_handle(newval);
	}
	if (event==FL_RELEASE)
	  moving=false;
	selected.clear();
	redraw();
	return 1;
      }
      return 0;
    }
    selected.clear();
    if (mode==1){
      if (function_final!=at_point){
	if (event==FL_RELEASE){
	  string args=autoname(contextptr)+":=";
	  if (function_final.type==_FUNC)
	    args += function_final._FUNCptr->ptr()->s;
	  args +="(";
	  if (function_final==at_plotode)
	    args += fcnfield + "," + fcnvars + "," +tmp.print(contextptr) + ",plan)";
	  else
	    args += tmp.print(contextptr) + ")";
	  autoname_plus_plus();
	  set_gen_value(-1,gen(args,contextptr),true);
	}
	if (event==FL_PUSH || event==FL_DRAG || event==FL_RELEASE)
	  return 1;
	return 0;
      }
      // point|segment mode
      if (event==FL_RELEASE){
	hp_pos=-1;
	if (hp && !args_tmp.empty() && (std::abs(push_i-current_i)>npixels || std::abs(push_j-current_j)>npixels || (is3d && std::abs(push_depth-current_depth) >0)) ){
	  // make a segment
	  gen val1,val2;
	  if (in_area && args_tmp.front().is_symb_of_sommet(at_point)){
	    val1=gen(autoname(contextptr),contextptr);
	    // put in last history pack level
	    set_gen_value(-1,symbolic(at_sto,makevecteur(add_attributs(args_tmp.front(),couleur,contextptr),val1)),false);
	    hp_pos=hp->elements.size()-1;
	    autoname_plus_plus();
	  }
	  else 
	    val1=args_tmp.front();
	  if (in_area && tmp.is_symb_of_sommet(at_point)){
	    val2=gen(autoname(contextptr),contextptr);
	    gen tmp3=symbolic(at_sto,makevecteur(add_attributs(tmp,couleur,contextptr),val2));
	    set_gen_value(-1,tmp3,false);
	    if (hp_pos<0) hp_pos=hp->elements.size()-1;
	    autoname_plus_plus();
	  }
	  else 
	    val2=tmp;
	  if (in_area){
	    gen tmp3=add_attributs(symbolic(at_segment,makevecteur(val1,val2)),couleur,contextptr);
	    string v1v2=val1.print(contextptr)+val2.print(contextptr);
	    gen g1g2(v1v2,contextptr);
	    if (g1g2.type!=_IDNT)
	      g1g2=gen(v1v2+"_",contextptr);
	    tmp3=symbolic(at_sto,makevecteur(tmp3,g1g2));
	    set_gen_value(-1,tmp3,false);
	    if (hp_pos<0) hp_pos=hp->elements.size()-1;
	    eval(hp_pos);
	  }
	  return 1;
	}
	if (in_area && tmp.type!=_IDNT)
	  do_handle(symbolic(at_sto,makevecteur(add_attributs(tmp,couleur,contextptr),gen(autoname(contextptr),contextptr))));
	// element
	if (tmp.type==_IDNT && tmp2.type==_SYMB && !equalposcomp(point_sommet_tab_op,tmp2._SYMBptr->sommet)){
	  // tmp2 is the geo object, find parameter value
	  double newx,newy,newz;
	  find_xyz(current_i,current_j,current_depth,newx,newy,newz);
	  round3(newx,window_xmin,window_xmax);
	  round3(newy,window_ymin,window_ymax);
	  gen t=projection(evalf(tmp2,1,contextptr),gen(newx,newy),contextptr);
	  if (is_undef(t))
	    return 0;
	  gen tmp3=symbolic(at_element,( (t.type<_IDNT || t.type==_VECT)?gen(makevecteur(tmp,t),_SEQ__VECT):tmp));
	  tmp3=symbolic(at_sto,makevecteur(add_attributs(tmp3,couleur,contextptr),gen(autoname(contextptr),contextptr)));
	  set_gen_value(-1,tmp3,false);
	  if (hp_pos<0) hp_pos=hp->elements.size()-1;
	  eval(hp_pos);
	}
	return 1;
      }
      if (event==FL_PUSH){
	args_tmp=vecteur(1,tmp);
	return 1;
      }
      if (event==FL_DRAG){
	redraw();
	return 1;
      }
      return 0;
    }
    gen tmpval=remove_at_pnt(tmp.eval(1,contextptr));
    gen somm=symbolic(at_sommets,tmp);
    int npoints=1;
    if (!equalposcomp(nosplit_polygon_function,*function_final._FUNCptr)){
      if (tmpval.type==_VECT && tmpval.subtype==_GROUP__VECT)
	npoints=tmpval._VECTptr->size();
      if (tmpval.is_symb_of_sommet(at_cercle))
	npoints=is3d?3:2;
    }
    unsigned args_size=args_tmp.size();
    // mode>=2
    if (event==FL_MOVE || event==FL_DRAG || event==FL_RELEASE || event==FL_PUSH){
      if (args_size<args_tmp_push_size)
	args_tmp_push_size=args_size;
      args_tmp.erase(args_tmp.begin()+args_tmp_push_size,args_tmp.end());
    }
    unsigned new_args_size=args_tmp.size();
    gen tmp_push=tmp;
    bool swapargs=false;
    if (npoints==2 && (new_args_size==2 || new_args_size==1) && (function_final==at_angleat || function_final==at_angleatraw) ){
      // search if args_tmp[0] or args_tmp[1] is a vertex of tmp
      gen tmp2=remove_at_pnt(evalf(tmp,1,contextptr));
      gen somm=symbolic(at_sommets,tmp);
      if (tmp2.type==_VECT && tmp2._VECTptr->size()==2){
	gen tmpa=remove_at_pnt(evalf(args_tmp[0],1,contextptr));
	gen tmpb=new_args_size==2?remove_at_pnt(evalf(args_tmp[1],1,contextptr)):undef;
	if (npoints==2 && tmpa==tmp2._VECTptr->front() && tmpb!=tmp2._VECTptr->back()){
	  tmp=symbolic(at_at,gen(makevecteur(somm,1),_SEQ__VECT));
	  npoints=1;
	}
	if (npoints==2 && tmpa==tmp2._VECTptr->back() && tmpb!=tmp2._VECTptr->front()){
	  tmp=symbolic(at_at,gen(makevecteur(somm,0),_SEQ__VECT));
	  npoints=1;
	}
	if (npoints==2 && tmpb==tmp2._VECTptr->front() && tmpa!=tmp2._VECTptr->back() ){
	  swapargs=true;
	  tmp=symbolic(at_at,gen(makevecteur(somm,1),_SEQ__VECT));
	  npoints=1;
	}
	if (npoints==2 && tmpb==tmp2._VECTptr->back() && tmpa!=tmp2._VECTptr->front()){
	  swapargs=true;
	  tmp=symbolic(at_at,gen(makevecteur(somm,0),_SEQ__VECT));
	  npoints=1;
	}
      }
    }
    if (npoints+args_tmp.size()>mode)
      npoints=1;
    if (event==FL_MOVE || event==FL_DRAG || event==FL_RELEASE){
      if (args_size && args_tmp_push_size && args_push!=tmp_push){
	// replace by current mouse position
	if (npoints==1)
	  args_tmp.push_back(tmp);
	else {
	  gen somm=symbolic(at_sommets,tmp);
	  for (int i=0;i<npoints;++i){
	    args_tmp.push_back(symbolic(at_at,gen(makevecteur(somm,i),_SEQ__VECT)));
	  }
	}
	redraw();
	if (event!=FL_RELEASE || (abs(push_i-current_i)<=5 && abs(push_j-current_j)<=5))
	  return 1;
      }
    }
    if (event==FL_PUSH){
      if (swapargs)
	swapgen(args_tmp[0],args_tmp[1]);
      args_push=tmp_push;
      if (npoints==1)
	args_tmp.push_back(tmp);
      else {
	for (int i=0;i<npoints;++i){
	  args_tmp.push_back(symbolic(at_at,gen(makevecteur(somm,i),_SEQ__VECT)));
	}
      }
      args_tmp_push_size=args_tmp.size();
      redraw();
      return 1;
    }
    if (event==FL_RELEASE){
      int s=args_tmp.size();
      args_tmp_push_size=s;
      if (mode>1 && s>=mode){
	if (s>mode){ 
	  args_tmp=vecteur(args_tmp.begin(),args_tmp.begin()+mode);
	  s=mode;
	}
	gen tmp_plot;
	if (in_area && function_final.type==_FUNC) {
	  gen res,objname=gen(autoname(contextptr),contextptr);
	  hp_pos=hp->elements.size();
	  if (hp_pos && hp->elements[hp_pos-1].s.empty())
	    --hp_pos;
	  // hp->update_pos=hp_pos;
	  int pos0=hp_pos;
	  unary_function_ptr * ptr=function_final._FUNCptr;
	  int ifinal=mode;
	  if (equalposcomp(measure_functions,*ptr))
	    ifinal--;
	  // first replace points in args_tmp by assignations
	  for (int i=0;i<ifinal;++i){
	    tmp_plot=args_tmp[i];
	    if (tmp_plot.is_symb_of_sommet(at_point)){
	      tmp_plot=symbolic(at_sto,makevecteur(add_attributs(tmp_plot,couleur,contextptr),objname));
	      args_tmp[i]=objname;
	      set_gen_value(hp_pos,tmp_plot,false);
	      add_entry(hp_pos+1);
	      ++hp_pos;
	      autoname_plus_plus();
	      objname=gen(autoname(contextptr),contextptr);
	    }
	  }
	  vecteur argv=args_tmp;
	  if (*ptr==(is3d?at_sphere:at_cercle)){
	    gen argv1;
#ifdef NO_STDEXCEPT
	    argv1=evalf(args_tmp.back(),1,contextptr);
	    argv1=evalf_double(argv1,1,contextptr);
#else
	    try {
	      argv1=evalf(args_tmp.back(),1,contextptr);
	      argv1=evalf_double(argv1,1,contextptr);
	    }
	    catch (std::runtime_error & e){
	      argv1=undef;
	    }
#endif
	    if (argv1.is_symb_of_sommet(at_pnt) ||argv1.type==_IDNT){
	      argv1=remove_at_pnt(argv1);
	      if ( (argv1.type==_VECT && argv1.subtype==_POINT__VECT) || argv1.type==_CPLX || argv1.type==_IDNT)
	      argv.back()=args_tmp.back()-args_tmp.front();
	    }
	  }
	  tmp_plot=symbolic(*ptr,gen(argv,_SEQ__VECT));
#ifdef NO_STDEXCEPT
	  res=evalf(tmp_plot,1,contextptr);
#else
	  try {
	    res=evalf(tmp_plot,1,contextptr);
	  }
	  catch (std::runtime_error & err){
	    res=undef;
	  }
#endif
	  tmp_plot=symbolic(at_sto,makevecteur(add_attributs(tmp_plot,couleur,contextptr),objname));
	  set_gen_value(hp_pos,tmp_plot,false);
	  add_entry(hp_pos+1);
	  ++hp_pos;
	  if (res.is_symb_of_sommet(at_pnt)){
	    res=remove_at_pnt(res);
	    int ns=0;
	    if (res.type==_VECT && res.subtype==_GROUP__VECT && (ns=res._VECTptr->size())>2){
	      vecteur l;
	      if (res._VECTptr->back()==res._VECTptr->front())
		--ns;
	      if (function_final.type==_FUNC && equalposcomp(transformation_functions,*function_final._FUNCptr)){
		vecteur argv;
		gen objn,som=symbolic(at_sommets,objname);
		for (int i=1;i<=ns;++i){
		  tmp_plot=symbolic(at_at,gen(makevecteur(som,i-1),_SEQ__VECT));
		  objn=gen(autoname(contextptr)+print_INT_(i),contextptr);
		  argv.push_back(objn);
		  set_gen_value(hp_pos,symbolic(at_sto,gen(makevecteur(tmp_plot,objn),_SEQ__VECT)),false);
		  add_entry(hp_pos+1);
		  ++hp_pos;
		}
		for (int i=1;i<=ns;++i){
		  tmp_plot=symbolic(at_segment,makevecteur(argv[i-1],argv[i%ns]));
		  set_gen_value(hp_pos,symbolic(at_sto,makevecteur(add_attributs(tmp_plot,couleur,contextptr),gen(autoname(contextptr)+print_INT_(ns+i),contextptr))),false);
		  add_entry(hp_pos+1);
		  ++hp_pos;
		}		
	      }
	      else {
		for (int i=mode;i<ns;++i){
		  tmp_plot=symb_at(
				   symbolic(at_sommets,gen(autoname(contextptr),contextptr))
				   ,i,contextptr);
		  gen newname=gen(autoname(contextptr)+print_INT_(i),contextptr);
		  set_gen_value(hp_pos,symbolic(at_sto,makevecteur(tmp_plot,newname)),false);
		  args_tmp.push_back(newname);
		  add_entry(hp_pos+1);
		  ++hp_pos;
		}
		for (int i=1;i<=ns;++i){
		  tmp_plot=symbolic(at_segment,makevecteur(args_tmp[i-1],args_tmp[i%ns]));
		  set_gen_value(hp_pos,symbolic(at_sto,makevecteur(add_attributs(tmp_plot,couleur,contextptr),gen(autoname(contextptr)+print_INT_(ns+i),contextptr))),false);
		  add_entry(hp_pos+1);
		  ++hp_pos;
		}
	      }
	    } // if res.type==_VECT
	  }
	  autoname_plus_plus();
	  // hp->undo_position=save_undo_position;
	  eval(pos0);
	}
	args_tmp.clear();
	args_tmp_push_size=0;
      }
      redraw();
      return 1;
    }
    return 0;
  }

  void geosave(textArea * text,GIAC_CONTEXT,bool writeflash){
    string S(merge_area(text->elements));
    string s=remove_extension(text->filename.c_str()+7);
    if (!inexammode && writeflash)
      save_script(text->filename,S.c_str());
    gen tmp(s,contextptr);
    if (tmp.type==_IDNT){
      sto(makevecteur(at_pnt,string2gen(S,false)),tmp,contextptr);
      return;
    }
    else {
      for (int i=0;i<10;++i){
	string s1=s+print_INT_(i);
	tmp=gen(s1,contextptr);
	if (tmp.type==_IDNT){
	  confirm(lang==1?"Nom de sauvegarde reserve":"Unable to use reserved name",((lang==1?"Nom utilise ":"Name used ")+s1).c_str());
	  sto(makevecteur(at_pnt,string2gen(merge_area(text->elements),false)),tmp,contextptr);
	  return;
	}
      }
    }
    confirm(lang==1?"Nom de sauvegarde reserve":"Unable to use reserved name",lang==1?"Sauvegarde impossible":"Unable to save");
  }

  void Graph2d::adjust_cursor_point_type(){
    if (hp){
      double newx,newy,newz;
      find_xyz(current_i,current_j,current_depth,newx,newy,newz);
      int pos=-1;
      gen orig;
      gen res=geometry_round(newx,newy,newz,find_eps(),orig,pos);
      if (mode==0){
	if (pos>=0)
	  selected=vector<int>(1,pos);
	else
	  selected.clear();
      }
      cursor_point_type=pos>=0?6:3;
    }
  }

  vecteur mark_selected(const vecteur & v,const vector<int> & selected,bool is3d){
    vecteur w(v);
    vector<int> s(selected); sort(s.begin(),s.end());
    int pos=0;
    for (int i=0;i<w.size();++i){
      if (pos>=s.size())
	break;
      if (i==s[pos]){
	++pos;
	gen g=w[i];
	if (g.is_symb_of_sommet(at_pnt)){
	  g=g._SYMBptr->feuille;
	  if (g.type==_VECT && g._VECTptr->size()>=2){
	    vecteur gv(*g._VECTptr);
	    gv[1]=is3d?_CYAN:_BLUE;
	    g=gen(gv,g.subtype);
	    w[i]=symbolic(at_pnt,g);
	  }
	}
      }
    }
    return w;
  }

  vecteur Graph2d::selected_names(bool allobjects,bool withdef) const {
    vector<int>::const_iterator it=selected.begin(),itend=selected.end();
    vecteur res;
    for (;it!=itend;++it){
      gen g=symbolic_instructions[*it];
      if (g.is_symb_of_sommet(at_sto)){
	gen tmp=g._SYMBptr->feuille[0];
	if (allobjects || tmp.is_symb_of_sommet(at_point) || tmp.is_symb_of_sommet(at_element))
	  res.push_back(withdef?g:g._SYMBptr->feuille[1]);
      }
    }
    return res;
  }

  void Graph2d::update_g(){
    if (hp){
      adjust_cursor_point_type();
      find_title_plot(title_tmp,plot_tmp);
      vecteur v(mergevecteur(get_current_animation(),trace_instructions));
      if (!is_undef(plot_tmp)) v.push_back(plot_tmp);
      // geometry: update g from plot_instructions
      g=mergevecteur(selected.empty()?plot_instructions:mark_selected(plot_instructions,selected,is3d),v);
      if (is3d)
	update_rotation();
    }
  }

  void geohelp(){
    textArea text;
    text.editable=false;
    text.clipline=-1;
    text.title = (char*)((lang==1)?"Aide":"Help");
    text.allowF1=false;
    text.python=false;
    add(&text,lang==1?
	"x,theta,t: infos courbes (uniquement en 2d)\nhaut/bas/droit/gauche: deplace pointeur ou change point de vue\nSIN: axes ON/OFF\n*: autoscale\n/:orthonormalise\n+- zoom\n\n3d\ny^x ou e^x: trace 3d precis\nF2: +rapide/-precis\nF3: -rapide/+precis\nEXIT: quitte ou interrompt le trace 3d en cours\n( et ): modifie le rendu des surfaces raides 3d\n0: surfaces cachees 3d ON/OFF\n.: remplissage surface 3d raide ON/OFF\n5 reset 3d view\n7,8,9,1,2,3: deplacement 3d\n\nGeometrie\nF4: change le mode\nLe mode repere (shift F1) permet de changer le point de vue\nLe mode pointeur (shift F2) permet de bouger un objet et les objets dependants avec EXE et les touches de deplacement\nLes autres modes permettent de creer des objets\nEXIT: permet de passer en vue symbolique et de creer/modifier des objets par des commandes, taper EXE pour revenir en vue graphique\n4,6: modifie la profondeur du clic":
	"x,theta,t: curves infos (2d only)\nup/down/right/left: move pointer or modify viewpoint\nSIN: axes ON/OFF\n*: autoscale\n:/: orthonormalize\n+- zoom\n\n3d\nON/Back: leave or interrupt 3d rendering\ny^x or e^x: precise 3d\nF2: faster&less precise\nF3: slower&more precise\n( and ): modify stiff surfaces 3d rendering\n0: hidden 3d surfaces ON/OFF\n.: fill stiff 3d surfacesON/OFF\n5 reset 3d view\n7,8,9,1,2,3: move 3d view\n\nGeometry\nF4: change geometry mode\nFrame mode (shift F1): modify viewpoint\nPointer mode (shift F2): select an object and move it with EXE and cursor keys\nOther modes: create an object\nEXIT: go to symbolic view where you can create/modify objects with commands, press EXE to go back to graphic view");
    int exec=doTextArea(&text);
  }

  int Graph2d::ui(){
    Graph2d & gr=*this;
    if (!hp && 
        gr.symbolic_instructions.size()==1 &&
        gr.symbolic_instructions.front().is_symb_of_sommet(at_plotfield)){
      if (newgeo(contextptr)==0){
        geoptr->hp->filename="\\\\fls0\\ode.py";
        if (geoptr!=this){
          geoptr->symbolic_instructions=symbolic_instructions;
          geoptr->hp->elements.clear();
          for (int i=0;i<symbolic_instructions.size();++i){
            geoptr->hp->set_string_value(i,symbolic_instructions[i].print(contextptr));
          }
          // switch to geo2d plotfield mode
          geoptr->set_mode(at_point,at_plotfield,1,"init_cond");
          double eps=epsilon(contextptr);
          epsilon(1e-6,contextptr);
          geoloop(geoptr);
          epsilon(eps,contextptr);
          return 0;
        }
      }
    }
    // UI
    int saveprecision=gr.precision;
    gr.precision += 2; // fast draw first
    gr.draw();
    gr.precision=saveprecision;    
    gr.must_redraw=true;
    for (;;){
      const char * msg="+-:zoom,F1:help,F6: menu,EXIT:quit";
      DefineStatusMessage((char *)msg,1,0,0);
      DisplayStatusArea();
      int saveprec=gr.precision;
      if (gr.doprecise){
	gr.doprecise=false;
	gr.precision=1;//gr.precision-=2;
      }
      if (gr.must_redraw)
	gr.draw();
      if (hp){
	string msg=hp->filename.c_str()+7;
	msg += ":";
	msg += mode==255?" Frame. F1: help":modestr;
	// help
	int help_pos=args_tmp.empty()?0:args_tmp.size()-1;
	if (help_pos<args_help.size()){
	  msg += " "+args_help[help_pos];
	}
	DefineStatusMessage((char *)msg.c_str(),1,0,0);
	DisplayStatusArea();
      }
      gr.must_redraw=true;
      gr.precision=saveprec;
      if (0 && !hp){
#ifdef NUMWORKS
	os_draw_string(0,LCD_HEIGHT_PX-STATUS_AREA_PX-17,COLOR_BLACK,COLOR_WHITE,"home: cfg");
#else
	os_draw_string_small(0,LCD_HEIGHT_PX-17,COLOR_BLACK,COLOR_WHITE,"F1: help, F6: cfg");
#endif
      }
      int key=-1;
#if 1
      ck_getkey(&key);
      bool alph=oldalphastate;
#else
      int keyflag=GetSetupSetting( (unsigned int)0x14);
      bool alph=keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
      ck_getkey(&key);
#endif
      if (key==KEY_SHUTDOWN)
	return key;
      if (key==KEY_CTRL_F1){
	geohelp();
	continue;
      }
      if (key==KEY_CTRL_F2 && !is3d){
	tracemode_set(-1); // object info
	continue;
      }
      if (key==KEY_CTRL_F3 && !is3d){
	if (tracemode & 2)
	  tracemode &= ~2;
	else
	  tracemode |= 2;
	tracemode_set();
	continue;
      }
      if (!hp && key==KEY_CTRL_F4){
	if (tracemode & 4)
	  tracemode &= ~4;
	else
	  tracemode |= 4;
	tracemode_set();
	continue;
      }
      if (key==KEY_CTRL_F5){
	if (tracemode & 8)
	  tracemode &= ~8;
	else {
	  tracemode |= 8;
	  orthonormalize(true);
	}
	tracemode_set();
	continue;
      }
      if (key==KEY_CTRL_XTT || key=='\t'){
	if (!is3d) curve_infos();
	continue;
      }
      if (!hp && key==KEY_CTRL_F7)
	invert_tracemode();
      if (hp){
	if (key==KEY_CTRL_F7 ){
	  if (mode==255)
	    invert_tracemode();
	  else
	    set_mode(0,0,255,"");
	}
	if (key==KEY_CTRL_F8)
	  set_mode(0,0,0,"");
	if (key==KEY_CTRL_F9)
	  set_mode(at_point,at_point,1,"Point");
	if (key==KEY_CTRL_F10)
	  set_mode(at_segment,is3d?at_sphere:at_cercle,2,"Center,Point");
	if (key==KEY_CTRL_F11)
	  set_mode(at_segment,at_triangle,3,"Point1,Point2,Point3");
	if (key>='a' && key<='z'){
	  bool found=false;
	  char ch=key;
	  gen tmp=gen(string("")+ch,contextptr);
	  if (tmp.type==_IDNT){
	    int pos=0;
	    for (int i=0;i<plot_instructions.size();++i){
	      if (plot_instructions[i].is_symb_of_sommet(at_parameter)){
		gen name=plot_instructions[i]._SYMBptr->feuille[0];
		++pos;
		if (name==tmp){
		  current_j=20+14*pos;
		  found=true;
		  break;
		}
	      }
	    }
	  }
	  if (found)
	    continue;
	  key -= 'a'-'A';
	}
	if (key>='A' && key<='Z'){
	  char ch=key;
	  gen tmp=gen(string("")+ch,contextptr);
	  if (tmp.type==_IDNT){
	    tmp=evalf(tmp,1,contextptr);
	    if (tmp.is_symb_of_sommet(at_pnt)){
	      tmp=remove_at_pnt(tmp);
	      if (tmp.is_symb_of_sommet(at_cercle))
		tmp=(tmp._SYMBptr->feuille[0]+tmp._SYMBptr->feuille[1])/2;
	      if (tmp.type==_SYMB)
		tmp=tmp._SYMBptr->feuille;
	      if (tmp.type==_VECT && tmp.subtype!=_POINT__VECT && !tmp._VECTptr->empty())
		tmp=tmp._VECTptr->front();
	      if (is3d && tmp.type==_VECT && tmp._VECTptr->size()==3 && tmp.subtype==_POINT__VECT){
		const vecteur & tv=*tmp._VECTptr;
		gen x=tv[0],y=tv[1],z=tv[2];
		x=evalf_double(x,1,contextptr); 
		y=evalf_double(y,1,contextptr); 
		z=evalf_double(z,1,contextptr);
		if (x.type==_DOUBLE_ && y.type==_DOUBLE_ && z.type==_DOUBLE_){
		  float3d i,j; float3d3 d3;
		  xyz2ij(float3d3(x._DOUBLE_val,y._DOUBLE_val,z._DOUBLE_val),i,j,d3);
		  current_i=i; current_j=j; current_depth=d3.z;
		}
	      }
	      if (!is3d && (tmp.type==_DOUBLE_ || tmp.type==_CPLX)){
		double x_scale=LCD_WIDTH_PX/(window_xmax-window_xmin);
		double y_scale=(LCD_HEIGHT_PX-STATUS_AREA_PX)/(window_ymax-window_ymin);
		double i,j;
		findij(tmp,x_scale,y_scale,i,j,contextptr);
		current_i=int(i+.5);
		current_j=int(j+.5);
		adjust_cursor_point_type();
		geo_handle(moving?FL_DRAG:FL_MOVE,key);
		continue;
	      }
	    }
	  }
	}
      }
      if (hp && (key==KEY_CTRL_CATALOG || key==KEY_BOOK || key==KEY_CTRL_F4)){
	const char *
	  tab[]={
		 lang==1?"Mode repere":"Frame mode", // 0
		 lang==1?"Pointeur":"Pointer",
		 lang==1?"Point":"Point", // 2
		 is3d?"Sphere":"Circle",
		 lang==1?"Triangle":"Triangle", // 4
		 lang==1?"Points":"Points",
		 lang==1?"Droites, plans":"Lines, planes", // 6
		 lang==1?"Polygone, polyedre":"Polygon, polyhedron",
		 lang==1?"Cercle, conique, sphere":"Circle, conic, sphere", // 8
		 lang==1?"Courbe, surface":"Curve, surface", // 9
		 lang==1?"Curseur":"Cursor", // 10
		 lang==1?"Transformations":"Transforms",
		 lang==1?"Mesures":"Mesures", // 12
		 lang==1?"Effacer trace":"Clear trace", // -1
		 0};
	const int s=sizeof(tab)/sizeof(char *);
	int choix=select_item(tab,"Mode",true);
	if (choix<0 || choix>s)
	  continue;
	if (choix==s-1){
	  trace_instructions.clear();
	  update_g();
	  continue;
	}
	if (choix<=4){
	  gen ftmp[]={0,0,at_point,at_segment,at_segment};
	  gen ffinal[]={0,0,at_point,is3d?at_sphere:at_cercle,at_triangle};
	  int mode[]={255,0,1,2,3};
	  const char * help[]={"","","Point","Center,Point","Point1,Point2,Point3"};
	  set_mode(ftmp[choix],ffinal[choix],mode[choix],help[choix]);
	  continue;
	}
	draw(); // for small choosebox, we must clean up previous choosebox
	if (choix==5){ // Points
	  const char *
	    tab[]={
		   lang==1?"Point":"Point",
		   lang==1?"Milieu":"Middle point",
		   lang==1?"Centre":"Center",
		   lang==1?"Intersection unique":"Single intersection",
		   lang==1?"Liste d'intersections":"List of intersections",
		   lang==1?"Element":"Element",
		   0};
	  const int s=sizeof(tab)/sizeof(char *);
	  int choix=select_item(tab,"Points",true);
	  if (choix<0 || choix>s)
	    continue;
	  gen ftmp[]={at_point,at_segment,at_centre,at_inter_unique,at_inter,at_element};
	  gen ffinal[]={at_point,at_milieu,at_centre,at_inter_unique,at_inter,at_element};
	  int mode[]={1,2,1,2,2,1};
	  const char * help[]={
			       "Point",
			       "Point1,Point2",
			       "Circle",
			       "Line1,Line2",
			       "Curve1,Curve2",
			       "Curve",
	  };
	  set_mode(ftmp[choix],ffinal[choix],mode[choix],help[choix]);
	  continue;
	}
	if (choix==6){ // Droites
	  const char *
	    tab[]={
		   lang==1?"Segment":"Segment", 
		   lang==1?"Vecteur":"Vector",
		   lang==1?"Demi-droite":"Halfline",
		   lang==1?"Droite":"Line",
		   lang==1?"Plan":"Plane",
		   lang==1?"Parallele":"Parallel",
		   lang==1?"Perpendiculaire":"Perpendicular",
		   lang==1?"Mediatrice":"Perpen_bisector",
		   lang==1?"Bissectrice":"Bisector",
		   lang==1?"Mediane":"Median line",
		   lang==1?"Tangente":"Tangent",
		   0};
	  const int s=sizeof(tab)/sizeof(char *);
	  int choix=select_item(tab,"Droites, segments...",true);
	  if (choix<0 || choix>s)
	    continue;
	  gen ftmp[]={at_segment,at_vector,at_demi_droite,at_droite,at_segment,at_parallele,at_perpendiculaire,at_mediatrice,at_segment,at_segment,at_segment};
	  gen ffinal[]={at_segment,at_vector,at_demi_droite,at_droite,at_plan,at_parallele,at_perpendiculaire,at_mediatrice,at_bissectrice,at_mediane,at_tangent};
	  int mode[]={2,2,2,2,2,3,2,2,3,3,2};
	  const char * help[]={
			       "Point1,Point2",
			       "Point1,Point2",
			       "Point1,Point2",
			       "Point1,Point2",
			       "Point1,Point2,Point3",
			       "Point,Line",
			       "Point,Line",
			       "Point1,Point2",
			       "Sommet_angle,Point2,Point3",
			       "Sommet_angle,Point2,Point3",
			       "Curve,Point"
	  };
	  set_mode(ftmp[choix],ffinal[choix],mode[choix],help[choix]);
	  continue;
	}
	if (choix==7){ // Polygons
	  const char *
	    tab[]={
		   lang==1?"Triangle":"Triangle",
		   lang==1?"Triangle equilateral":"Equilateral triangle",
		   lang==1?"Carre":"Square",
		   lang==1?"Quadrilatere":"Quadrilateral",
		   lang==1?"Polygone":"Polygon",
		   lang==1?"Tetraedre (pyramide)":"Tetrahedron (Pyramid)",
		   lang==1?"Tetraedre regulier":"Regular tetrahedron",
		   lang==1?"Cube":"Cube",
		   lang==1?"Octaedre":"Octahedron",
		   lang==1?"Dodecaedre":"Dodecahedron",
		   lang==1?"Icosaedre":"Icosahedron",
		   0};
	  const int s=sizeof(tab)/sizeof(char *);
	  int choix=select_item(tab,"Droites, segments...",true);
	  if (choix<0 || choix>s)
	    continue;
	  gen ftmp[]={at_polygone_ouvert,at_segment,at_segment,at_polygone_ouvert,at_polygone_ouvert,at_polygone_ouvert,at_polygone_ouvert,at_polygone_ouvert,at_polygone_ouvert,at_polygone_ouvert,at_polygone_ouvert};
	  gen ffinal[]={at_triangle,at_triangle_equilateral,at_carre,at_quadrilatere,at_polygone,at_tetraedre,at_tetraedre,at_cube,at_octaedre,at_dodecaedre,at_icosaedre};
	  int mode[]={3,2,2,4,5,4,3,3,3,3,3};
	  int m=mode[choix];
	  if (choix==4){
	    double d=5;
	    if (inputdouble(lang==1?"Nombre de sommets?":"Number of vertices?",d,contextptr) && d==int(d) && d>=3 && d<20){
	      m=d;
	    }
	    else continue;
	  }
	  const char * help[]={
			       "Point1,Point2,Point3",
			       "Point1,Point2",
			       "Point1,Point2",
			       "Point1,Point2,Point3,Point4",
			       "Point1,Point2,Point3,Point4,Point5",
			       "Point1,Point2,Point3,Point4",
			       "Point1,Point2,Point3",
			       "Point1,Point2,Point3",
			       "Point1,Point2,Point3",
			       "Point1,Point2,Point3",
			       "Point1,Point2,Point3",
	  };
	  set_mode(ftmp[choix],ffinal[choix],m,help[choix]);
	  continue;
	}
	if (choix==8){ // Conics
	  const char *
	    tab[]={
		   lang==1?"cercle":"circle",
		   lang==1?"circonscrit":"circumcircle",
		   lang==1?"inscrit":"incircle",
			   lang==1?"ellipse":"ellipse",
			   lang==1?"hyperbole":"hyperbola",
			   lang==1?"parabole":"parabola",
#ifndef CASCAS_ALEVEL_ONLY
			   lang==1?"sphere":"sphere",
#endif
			   0};
	  const int s=sizeof(tab)/sizeof(char *);
	  int choix=select_item(tab,"Conic",true);
	  if (choix<0 || choix>s)
	    continue;
		  gen ftmp[]={at_segment,at_segment,at_segment,at_segment,at_segment,at_segment
#ifndef CASCAS_ALEVEL_ONLY
		    ,at_segment
#endif
		  };
		  gen ffinal[]={at_cercle,at_circonscrit,at_inscrit,at_ellipse,at_hyperbole,at_parabole
#ifndef CASCAS_ALEVEL_ONLY
		    ,at_sphere
#endif
		  };
		  int mode[]={3,3,2,2,3,3
#ifndef CASCAS_ALEVEL_ONLY
		    ,2
#endif
		  };
		  const char * help[]={
				       "Center,Point",
				       "Point1,Point2,Point3",
				       "Point1,Point2,Point3",
				       "Focus1,Focus2,Point_on_ellipse",
				       "Focus1,Focus2,Point_on_hyperbola",
				       "Focus,Point_or_line",
#ifndef CASCAS_ALEVEL_ONLY
				       "Center,Point",
#endif
		  };
	  set_mode(ftmp[choix],ffinal[choix],mode[choix],help[choix]);
	  continue;
	}
	if (choix==9){ // Curves
	  const char *
	    tab[]={
		   lang==1?"_rmh64":"_rmh64",
		   lang==1?"Param. plotparam([x^2,x^3])":"Param. plotparam([x^2,x^3])",
		   lang==1?"Polaire plotpolar(x)":"Polar plotpolar(x)",
		   lang==1?"Implicit plot(x^2+y^4=6)":"Implicit plot(x^2+y^4=6)",
		   lang==1?"Champ des tangentes":"Plotfield",
		   lang==1?"Solution equa. diff.":"Diff. equa. solution",
		   0};
	  const int s=sizeof(tab)/sizeof(char *);
	  int choix=select_item(tab,"Courbe",true);
	  if (choix<0 || choix>s)
	    continue;
	  const char * cmd[]={"plot()","plotparam()","plotpolar()","plot()","plotfield()","plotode()"};
	  hp->line=hp->add_entry(-1);
	  string mycmd=autoname(contextptr)+":="+cmd[choix];
	  autoname_plus_plus();
	  hp->set_string_value(hp->line,mycmd);
	  hp->pos=mycmd.size()-1;
	  return KEY_CTRL_OK;
	}
	if (choix==10){
	  gen param=0;
	  for (char ch='a';ch<='z';++ch){
	    gen tmp(string("")+ch,contextptr);
	    if (tmp.type!=_IDNT) continue;
	    param=tmp.eval(1,contextptr);
	    if (param==tmp)
	      break;
	  }
	  if (param==0){
	    confirm(lang==1?"Plus de variables libres.":"No more free variable available",lang==1?"Essayez purge(a) ou purge(b) ou ...":"Try purge(a) or purge(b) or ...");
	    continue;
	  }
	  string mycmd=inputparam(param.print()[0],0);
	  if (!mycmd.empty()){
	    hp->line=hp->add_entry(-1);
	    hp->set_string_value(hp->line,mycmd);
	    hp->pos=mycmd.size()-1;
	  }
	  return KEY_CTRL_OK;	  
	}
	if (choix==11){ // Transforms
	  const char *
	    tab[]={
		   lang==1?"symetrie":"reflexion",
		   lang==1?"rotation":"rotation",
		   lang==1?"translation":"translation",
		   lang==1?"projection":"projection",
		   lang==1?"homothetie":"homothety",
		   lang==1?"similitude":"similarity",
		   // lang==1?"":"",
		   // lang==1?"":"",
		   0};
	  const int s=sizeof(tab)/sizeof(char *);
	  int choix=select_item(tab,"Transform",true);
	  if (choix<0 || choix>s)
	    continue;
	  gen ftmp[]={at_segment,at_polygone_ouvert,at_segment,at_segment,at_segment,at_polygone_ouvert};
	  gen ffinal[]={at_symetrie,at_rotation,at_translation,at_projection,at_homothetie,at_similitude};
	  int mode[]={2,3,2,2,2,3};
	  const char * help[]={
			       "Symmetry_center_axis,Object",
			       "Center,Angle,Object",
			       "Vector,Object",
			       "Curve,Object",
			       "Center,Ratio,Object",
			       "Center,Ratio,Angle,Object"
	  };
	  set_mode(ftmp[choix],ffinal[choix],mode[choix],help[choix]);
	  continue;
	}
	if (choix==12){ // Mesures
	  const char *
	    tab[]={
		   lang==1?"distance":"distance",
		   lang==1?"angle":"angle",
		   lang==1?"aire":"area",
		   lang==1?"perimetre":"perimeter",
		   lang==1?"pente":"slope",
		   lang==1?"distance seule":"distance raw",
		   lang==1?"angle seul":"angle raw",
		   lang==1?"aire seule":"area raw",
		   lang==1?"perimetre seul":"perimeter raw",
		   lang==1?"pente seule":"slope raw",
		   0};
	  const int s=sizeof(tab)/sizeof(char *);
	  int choix=select_item(tab,"Mesures",true);
	  if (choix<0 || choix>s)
	    continue;
	  gen ftmp[]={at_segment,at_triangle,at_areaat,at_perimeterat,at_slopeat,at_segment,at_triangle,at_areaatraw,at_perimeteratraw,at_slopeatraw};
	  gen ffinal[]={at_distanceat,at_angleat,at_areaat,at_perimeterat,at_slopeat,at_distanceatraw,at_angleatraw,at_areaatraw,at_perimeteratraw,at_slopeatraw};
	  int mode[]={3,4,2,2,2,3,4,2,2,2};
	  const char * help[]={
			       "Object1,Object2,Position",
			       "Angle_vertex,Direction1,Direction2,Position",
			       "Object,Position",
			       "Object,Position",
			       "Object,Position",
			       "Object1,Object2,Position",
			       "Angle_vertex,Direction1,Direction2,Position",
			       "Object,Position",
			       "Object,Position",
			       "Object,Position",
	  };
	  set_mode(ftmp[choix],ffinal[choix],mode[choix],help[choix]);
	  continue;
	}
	continue;
      }      

      if (key==KEY_CTRL_MENU || key==KEY_CTRL_F6 || (!hp && key==KEY_CTRL_F4)){
	char menu_xmin[32],menu_xmax[32],menu_ymin[32],menu_ymax[32],menu_zmin[32],menu_zmax[32],menu_depth[32];
	for (;;){
	  string s;
	  s="xmin "+print_DOUBLE_(gr.window_xmin,contextptr);
	  strcpy(menu_xmin,s.c_str());
	  s="xmax "+print_DOUBLE_(gr.window_xmax,contextptr);
	  strcpy(menu_xmax,s.c_str());
	  s="ymin "+print_DOUBLE_(gr.window_ymin,contextptr);
	  strcpy(menu_ymin,s.c_str());
	  s="ymax "+print_DOUBLE_(gr.window_ymax,contextptr);
	  strcpy(menu_ymax,s.c_str());
	  s="zmin 3d "+print_DOUBLE_(gr.window_zmin,contextptr);
	  strcpy(menu_zmin,s.c_str());
	  s="zmax 3d "+print_DOUBLE_(gr.window_zmax,contextptr);
	  strcpy(menu_zmax,s.c_str());
	  s="depth 3d "+print_DOUBLE_(gr.current_depth,contextptr);
	  //s=print_INT_(current_i)+","+print_INT_(current_j)+","+print_DOUBLE_(gr.current_depth,contextptr);
	  strcpy(menu_depth,s.c_str());
	  Menu smallmenu;
	  smallmenu.numitems=19;
	  MenuItem smallmenuitems[smallmenu.numitems];
	  smallmenu.items=smallmenuitems;
	  smallmenu.height=8;
	  //smallmenu.title = "KhiCAS";
	  smallmenuitems[0].text = (char *) ((lang==1)?"Aide":"Help");
	  smallmenuitems[1].text = (char*) ((lang==1)?"Etude graphe (xtt)":"Curve study (xtt)");
	  smallmenuitems[1].type = MENUITEM_CHECKBOX;
	  smallmenuitems[1].value = gr.tracemode;
	  smallmenuitems[2].text = (char *) menu_xmin;
	  smallmenuitems[3].text = (char *) menu_xmax;
	  smallmenuitems[4].text = (char *) menu_ymin;
	  smallmenuitems[5].text = (char *) menu_ymax;
	  smallmenuitems[6].text = (char *) menu_zmin;
	  smallmenuitems[7].text = (char *) menu_zmax;
	  smallmenuitems[8].text = (char *) menu_depth;
	  smallmenuitems[9].text = (char*) (lang==1?"Sauvegarder figure":"Save figure");
	  smallmenuitems[10].text = (char*) (lang==1?"Sauvegarder comme":"Save as");
	  smallmenuitems[11].text = (char*)((lang==1)?"Quitter":"Quit");
	  smallmenuitems[12].text = (char*) "Orthonormalize /";
	  smallmenuitems[13].text = (char*) "Autoscale *";
	  smallmenuitems[14].text = (char *) ("Zoom in +");
	  smallmenuitems[15].text = (char *) ("Zoom out -");
	  smallmenuitems[16].text = (char *) ("Y-Zoom out (-)");
	  smallmenuitems[17].text = (char*) ((lang==1)?"Voir axes":"Show axes");
	  smallmenuitems[17].type = MENUITEM_CHECKBOX;
	  smallmenuitems[17].value = gr.show_axes;
	  smallmenuitems[18].text = (char*) ((lang==1)?"Effacer traces geometrie":"Clear geometry traces");
	  drawRectangle(0,180,LCD_WIDTH_PX,60,_BLACK);
	  int sres = doMenu(&smallmenu);
	  if (sres == MENU_RETURN_EXIT)
	    break;
	  if (sres == MENU_RETURN_SELECTION || sres==KEY_CTRL_EXE) {
	    const char * ptr=0;
	    string s1; double d;
	    if (smallmenu.selection==1){
	      geohelp();
	      continue;
	      // gr.q=quaternion_double(0,0,0); gr.update();
	    }
	    if (smallmenu.selection==2)
	      gr.curve_infos();
	    if (smallmenu.selection==3){
	      d=gr.window_xmin;
	      if (inputdouble(menu_xmin,d,200)){
		gr.window_xmin=d;
		gr.update();
	      }
	    }
	    if (smallmenu.selection==4){
	      d=gr.window_xmax;
	      if (inputdouble(menu_xmax,d,200)){
		gr.window_xmax=d;
		gr.update();
	      }
	    }
	    if (smallmenu.selection==5){
	      d=gr.window_ymin;
	      if (inputdouble(menu_ymin,d,200)){
		gr.window_ymin=d;
		gr.update();
	      }
	    }
	    if (smallmenu.selection==6){
	      d=gr.window_ymax;
	      if (inputdouble(menu_ymax,d,200)){
		gr.window_ymax=d;
		gr.update();
	      }
	    }
	    if (smallmenu.selection==7){
	      d=gr.window_zmin;
	      if (inputdouble(menu_zmin,d,200)){
		gr.window_zmin=d;
		gr.update();
	      }
	    }
	    if (smallmenu.selection==8){
	      d=gr.window_zmax;
	      if (inputdouble(menu_zmax,d,200)){
		gr.window_zmax=d;
		gr.update();
	      }
	    }
	    if (smallmenu.selection==9){
	      d=gr.current_depth;
	      if (inputdouble(menu_depth,d,200)){
		if (d<-1) d=-1;
		if (d>1) d=1;
		gr.current_depth=d;
		gr.update();
	      }
	    }
	    if (hp && smallmenu.selection==10){
	      // save
	      geosave(hp,contextptr);
	      continue;
	    }
	    if (smallmenu.selection==11){
	      // save as
	      char filename[MAX_FILENAME_SIZE+1];
	      if (get_filename(filename,".py")){
		geoptr->hp->filename=filename;
                geosave(hp,contextptr,true /* write flash*/);
                continue;
              }
	    }
	    if (smallmenu.selection==12)
	      return -4;
	    if (smallmenu.selection==13)
	      gr.orthonormalize();
	    if (smallmenu.selection==14)
	      gr.autoscale();	
	    if (smallmenu.selection==15)
	      gr.zoom(0.7);	
	    if (smallmenu.selection==16)
	      gr.zoom(1/0.7);	
	    if (smallmenu.selection==17)
	      gr.zoomy(1/0.7);
	    if (smallmenu.selection==18)
	      gr.show_axes=!gr.show_axes;	
	    if (smallmenu.selection==19){
	      gr.trace_instructions.clear();
	      update_g();
	    }
	  }
	}
	gr.draw();
	gr.must_redraw=false;
	continue;
      }

      if (hp && key==KEY_CTRL_OK){
	if (mode==255)
	  return key;
	if (!moving){
	  pushed=true;
	  push_i=current_i;
	  push_j=current_j;
	  push_depth = current_depth;
	  geo_handle(FL_PUSH,KEY_CTRL_OK);
	  if (moving){
	    update_g();
	    continue;
	  }
	}
	int res=geo_handle(FL_RELEASE,KEY_CTRL_OK);
	pushed=false;
	update_g();
	continue;
      }
      if (hp && key==KEY_CTRL_EXIT && mode!=255){
	if (mode==0){ // restore original value and reeval
	  geo_handle(FL_RELEASE,KEY_CTRL_EXIT);
	  // do_handle(symbolic(at_sto,makevecteur(drag_original_value,drag_name)));
	  if (!pushed)
	    set_mode(0,0,255,"");
	}
	else
	  if (args_tmp.empty())
	    set_mode(0,0,255,"");
	pushed=false;
	moving=moving_frame=false;
	args_tmp.clear();
	update_g();
	continue;
      }
      if (key==KEY_CTRL_EXIT || key==KEY_CTRL_OK){
	return key;
      }
      if (key=='>'|| key==KEY_CTRL_F2){ // shift-+
	if (gr.is3d && gr.precision<9)
	  gr.precision++;
      }
      if (key=='\\' || key=='<'|| key==KEY_CTRL_F3){ // shift--
	if (gr.is3d && gr.precision>1)
	 gr.precision--;
      }
      const int t12=50; // 50/128 seconds
      if (key==KEY_CTRL_UP){
	if (tracemode && !alph){
	  --tracemode_n;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  --current_j;
	  if (current_j<0){
	    gr.up((gr.window_ymax-gr.window_ymin)/5);
	    current_j += (LCD_HEIGHT_PX-STATUS_AREA_PX)/5;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	if (gr.is3d){
	  int curprec=gr.precision;
	  gr.precision += 2;
	  if (gr.precision>9) gr.precision=9;
	  while (1){
	    //double X,Y,Z;
	    //do_transform(gr.invtransform,0.707,0.707,0,X,Y,Z);
	    //normalize(X,Y,Z);
	    //gr.q=rotation_2_quaternion_double(X,Y,Z,15)*gr.q;
	    //gr.q=quaternion_double(15,0,15)*gr.q;
	    gr.q=rotation_2_quaternion_double(0.707,0.707,0,15)*gr.q;// quaternion_double(15,0,0)*gr.q;
	    gr.update_rotation();
	    int t1=RTC_GetTicks();
	    gr.draw();
	    gr.must_redraw=gr.solid3d;
	    if (in_ckgetkey(&key,false,0,0,0,0)<0 || key!=KEY_CTRL_UP)
	      break;
	    int t2=RTC_GetTicks();
	    if (t2-t1<t12)
	      wait_1ms(100);
	  }
	  gr.precision=curprec;
	  continue;
	}
	gr.up((gr.window_ymax-gr.window_ymin)/16);
	continue;
      }
      if (key==KEY_CTRL_PAGEUP) {
	if (tracemode && !alph){
	  tracemode_n-=2;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  current_j-=(LCD_HEIGHT_PX-STATUS_AREA_PX)/5;
	  if (current_j<0){
	    gr.up((gr.window_ymax-gr.window_ymin)/2);
	    current_j += (LCD_HEIGHT_PX-STATUS_AREA_PX)/2;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	gr.up((gr.window_ymax-gr.window_ymin)/4);
	continue;
      }
      if (key==KEY_CTRL_DOWN) {
	if (tracemode && !alph){
	  ++tracemode_n;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  ++current_j;
	  if (current_j>=LCD_HEIGHT_PX-STATUS_AREA_PX){
	    gr.down((gr.window_ymax-gr.window_ymin)/5);
	    current_j -= (LCD_HEIGHT_PX-STATUS_AREA_PX)/5;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	if (gr.is3d){
	  int curprec=gr.precision;
	  gr.precision += 2;
	  if (gr.precision>9) gr.precision=9;
	  while (1){
	    //double X,Y,Z;
	    //do_transform(gr.invtransform,0.707,0.707,0,X,Y,Z);
	    //normalize(X,Y,Z);
	    //gr.q=rotation_2_quaternion_double(X,Y,Z,-15)*gr.q;
	    // gr.q=quaternion_double(-15,0,-15)*gr.q;
	    gr.q=rotation_2_quaternion_double(0.707,0.707,0,-15)*gr.q; // quaternion_double(-15,0,0)*gr.q;
	    gr.update_rotation();
	    int t1=RTC_GetTicks();
	    gr.draw();
	    gr.must_redraw=gr.solid3d;
	    if (in_ckgetkey(&key,false,0,0,0,0)<0 || key!=KEY_CTRL_DOWN)
	      break;
	    int t2=RTC_GetTicks();
	    if (t2-t1<t12)
	      wait_1ms(100);
	  }
	  gr.precision=curprec;
	  continue;
	}
	gr.down((gr.window_ymax-gr.window_ymin)/16);
	continue;
      }
      if (key==KEY_CTRL_PAGEDOWN) {
	if (tracemode && !alph){
	  tracemode_n+=2;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  current_j += (LCD_HEIGHT_PX-STATUS_AREA_PX)/5;
	  if (current_j>=LCD_HEIGHT_PX-STATUS_AREA_PX){
	    gr.down((gr.window_ymax-gr.window_ymin)/2);
	    current_j -= (LCD_HEIGHT_PX-STATUS_AREA_PX)/2;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	gr.down((gr.window_ymax-gr.window_ymin)/4);
      }
      if (key==KEY_CTRL_LEFT) {
	if (tracemode && !alph){
	  if (tracemode_i!=int(tracemode_i))
	    tracemode_i=std::floor(tracemode_i);
	  else
	    --tracemode_i;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  --current_i;
	  if (current_i<0){
	    gr.left((gr.window_xmax-gr.window_xmin)/5);
	    current_i += LCD_WIDTH_PX/5;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	if (gr.is3d){
	  int curprec=gr.precision;
	  gr.precision += 2;
	  if (gr.precision>9) gr.precision=9;
	  while (1){
	    gr.q=quaternion_double(0,15,0)*gr.q;
	    gr.update_rotation();
	    int t1=RTC_GetTicks();
	    gr.draw();
	    gr.must_redraw=gr.solid3d;
	    if (in_ckgetkey(&key,false,0,0,0,0)<0 || key!=KEY_CTRL_LEFT)
	      break;
	    int t2=RTC_GetTicks();
	    if (t2-t1<t12)
	      wait_1ms(100);
	  }
	  gr.precision=curprec;
	  continue;
	}
	gr.left((gr.window_xmax-gr.window_xmin)/16);
      }
      if (key==KEY_SHIFT_LEFT) {
	if (tracemode && !alph){
	  tracemode_i-=5;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  current_i -= LCD_WIDTH_PX/5;
	  if (current_i<0){
	    gr.left((gr.window_xmax-gr.window_xmin)/2);
	    current_i += LCD_WIDTH_PX/2;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	gr.left((gr.window_xmax-gr.window_xmin)/4);
	continue;
      }
      if (key==KEY_CTRL_RIGHT) {
	if (tracemode && !alph){
	  if (int(tracemode_i)!=tracemode_i)
	    tracemode_i=std::ceil(tracemode_i);
	  else
	    ++tracemode_i;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  ++current_i;
	  if (current_i>=LCD_WIDTH_PX){
	    gr.right((gr.window_xmax-gr.window_xmin)/5);
	    current_i -= LCD_WIDTH_PX/5;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	if (gr.is3d){
	  int curprec=gr.precision;
	  gr.precision += 2;
	  if (gr.precision>9) gr.precision=9;
	  while (1){
	    gr.q=quaternion_double(0,-15,0)*gr.q;
	    gr.update_rotation();
	    int t1=RTC_GetTicks();
	    gr.draw();
	    gr.must_redraw=gr.solid3d;
	    if (in_ckgetkey(&key,false,0,0,0,0)<0 || key!=KEY_CTRL_RIGHT)
	      break;
	    int t2=RTC_GetTicks();
	    if (t2-t1<t12)
	      wait_1ms(100);
	  }
	  gr.precision=curprec;
	  continue;
	}
	gr.right((gr.window_xmax-gr.window_xmin)/16);
	continue;
      }
      if (key==KEY_SHIFT_RIGHT) {
	if (tracemode && !alph){
	  tracemode_i+=5;
	  tracemode_set();
	  continue;
	}
	if (hp && mode!=255){
	  current_i += LCD_WIDTH_PX/5;
	  if (current_i>=LCD_WIDTH_PX){
	    gr.right((gr.window_xmax-gr.window_xmin)/2);
	    current_i -= LCD_WIDTH_PX/2;
	  }
	  geo_handle(moving?FL_DRAG:FL_MOVE,key);
	  update_g();
	  continue;
	}
	gr.right((gr.window_xmax-gr.window_xmin)/4);
	continue;
      }
      if (key==KEY_CHAR_PLUS) {
	gr.zoom(0.7);
      }
      if (key==KEY_CHAR_MINUS){
	gr.zoom(1/0.7);
      }
      if (key==KEY_CHAR_PMINUS){
	gr.zoomy(1/0.7);
      }
      if (key==KEY_CHAR_MULT){
	gr.autoscale();
      }
      if (key==KEY_CHAR_DIV) {
	gr.orthonormalize();
      }
      if (gr.is3d){
	if (key==KEY_CHAR_0){
	  gr.hide2nd=!gr.hide2nd;
	}
	if (key==KEY_CHAR_DP){
	  gr.interval=!gr.interval;
	}
	if (key==KEY_CHAR_ANS){
	  gr.show_edges=!gr.show_edges;
	}
	if (key==KEY_CHAR_4){
	  if (current_depth>-1)
	    current_depth-=0.1;
	}
	if (key==KEY_CHAR_6){
	  if (current_depth<1)
	    current_depth+=0.1;
	}
	if (key==KEY_CHAR_5){
	  gr.q=quaternion_double(0,0,0);
	  gr.update();
	}	  
	if (key==KEY_CHAR_8){
	  gr.z_up((gr.window_zmax-gr.window_zmin)/5);
	  gr.update_rotation();
	}
	if (key==KEY_CHAR_2){
	  gr.z_down((gr.window_zmax-gr.window_zmin)/5);
	  gr.update_rotation();
	}
	if (key==KEY_CHAR_1){
	  gr.right((gr.window_xmax-gr.window_xmin)/5);
	  gr.update_rotation();
	}
	if (key==KEY_CHAR_9){
	  gr.left((gr.window_xmax-gr.window_xmin)/5);
	  gr.update_rotation();
	}
	if (key==KEY_CHAR_3){
	  gr.up((gr.window_ymax-gr.window_ymin)/5);
	  gr.update_rotation();
	}
	if (key==KEY_CHAR_7){
	  gr.down((gr.window_ymax-gr.window_ymin)/5);
	  gr.update_rotation();
	}
	if (key==KEY_CHAR_POW || key==KEY_CHAR_EXPN)
	  gr.doprecise=true;
	if (key==KEY_CHAR_LPAR && gr.diffusionz<64)
	  gr.diffusionz++;
	if (key==KEY_CHAR_RPAR && gr.diffusionz>2)
	  gr.diffusionz--;
      }
      if (key==KEY_CHAR_SIN) {
	gr.show_axes=!gr.show_axes;
	gr.must_redraw=true;
	gr.draw();
	gr.must_redraw=false;
      }
      if (key==KEY_CTRL_VARS){
	select_var();
	gr.must_redraw=true;
	gr.draw();
	gr.must_redraw=false;
      }
    }
    // aborttimer = Timer_Install(0, check_execution_abort, 100); if (aborttimer > 0) { Timer_Start(aborttimer); }
    return 0;
  }

  // Turtle
  inline void swap_double(double & t1,double &t2){
    double t=t1;t1=t2;t2=t;
  }

  inline double min_double(double a,double b){
    return a<b?a:b;
  }

  inline double max_double(double a,double b){
    return a>b?a:b;
  }

  bool smaller_angle(double x,double y,double theta,double cottheta){
    double X=y*cottheta;
    // return true if angle(x,y)<theta
    if (x<=0){
      if (y<=0){
        if (theta>=-M_PI/2)
          return true;
        return x<X;
      }
      if (theta<=M_PI/2)
        return false;
      return X<x;
    }
    // now x>0
    if (y<0){
      if (theta<=-M_PI/2)
        return false;
      if (theta>=0)
        return true;
      return x<X;
    }
    if (theta<=0)
      return false;
    if (theta>=M_PI/2)
      return true;
    return X<x;
  }

  int my_round(double x){
    return int(x+.5);
  }

  void draw_turtle(double x1,double x2,double xc,double y,double yc,double theta1,double theta2,double cottheta1, double cottheta2, int c,GIAC_CONTEXT){
    int Y=my_round(yc-y);
    if (theta1==-M_PI && theta2==M_PI)
      draw_line(my_round(xc+x1),Y,my_round(xc+x2),Y,c);
    if (theta1>=0 && y<0)
      return;
    if (theta2<=0 && y>0)
      return;
    if (x1==x2) return;
    if (x1>x2) swap_double(x1,x2);
    if (y>0)
      swap_double(x1,x2);
    // x1<x2, if y<0 ;  x1>x2 if y>0
    bool test1,test2,test3,test4;
    if (y==0){
      double t1=std::atan2(y,x1),t2=std::atan2(y,x2);
      test1=t2<theta1; test2=t1>theta2;
      test3=t1<theta1; test4=t2>theta2;
    }
    else {
      test1=smaller_angle(x2,y,theta1,cottheta1);
      test2=!smaller_angle(x1,y,theta2,cottheta2);
      test3=smaller_angle(x1,y,theta1,cottheta1);
      test4=!smaller_angle(x2,y,theta2,cottheta2);
    }
    if (test1 || test2)
      return;
    if (!test3 && !test4) // (t1>=theta1 && t2<=theta2)
      draw_line(my_round(xc+x1),Y,my_round(xc+x2),Y,c);
    else {
      double X1=x1,X2=x2;
      if (test3) // (t1<theta1)
        X1=y*cottheta1;
      if (test4) // (t2>theta2)
        X2=y*cottheta2;
      draw_line(my_round(xc+X1),Y,my_round(xc+X2),Y,c);
    }
  }    

  void draw_turtle_arc(double xc,double yc,double r1,double r2,int c,double theta1, double theta2,GIAC_CONTEXT){
    if (theta1>theta2)
      swap_double(theta1,theta2);
    if (theta1==theta2)
      return;
    if (theta2-theta1>=360){
      theta1=-M_PI;
      theta2=M_PI;
    }
    else {
      int k=theta1/360;
      theta1 -= k*360;
      theta2 -= k*360;
      if (theta1>180){
        theta1 -= 360;
        theta2 -= 360;
      }
      if (theta2>180){
        draw_turtle_arc(xc,yc,r1,r2,c,theta1,180,contextptr);
        theta1 = -180;
        theta2 -= 360;
      }
      theta1=theta1/180*M_PI;
      theta2=theta2/180*M_PI;
    }
    if (theta1==0) theta1=-1e-10;
    if (theta2==0) theta2=1e-10;
    double cotheta1=std::cos(theta1)/std::sin(theta1);
    double cotheta2=std::cos(theta2)/std::sin(theta2);
    double r12=r1*r1,r22=r2*r2;
#if 1
    int y=std::floor(r2),xmax1=-1,xmax2=0;
    double delta1=0,delta2=r22-y*y;
    for (int y=r2;y>=0;--y){
      if (xmax1<0 && y<=r1){
        delta1=r12-y*y;
        xmax1=std::floor(std::sqrt(delta1));
        delta1=r12-y*y-xmax1*xmax1;
        while (delta1>0){
          ++xmax1;
          delta1 -= 2*xmax1+1;
        }
      }
      if (xmax1<0){
        draw_turtle(-xmax2,xmax2,xc,y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
        if (y)
          draw_turtle(-xmax2,xmax2,xc,-y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
      }
      else {
        draw_turtle(-xmax2,-xmax1,xc,y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
        draw_turtle(xmax1,xmax2,xc,y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
        if (y){
          draw_turtle(-xmax2,-xmax1,xc,-y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
          draw_turtle(xmax1,xmax2,xc,-y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
        }
        // update xmax1
        delta1 += 2*y-1;
        while (delta1>0){
          ++xmax1;
          delta1 -= 2*xmax1+1;
        }
      }
      // update xmax2
      delta2 += 2*y-1;
      while (delta2>0){
        ++xmax2;
        delta2 -= 2*xmax2+1;
      }
    }
#else
    for (double y=-r2;y<=r2;++y){
      // draw if (x-xc)^2+(y-yc)^2 is in [r1^2,r2^2]
      // i.e. (x-xc)^2 in [r1^2-dy2,r2^2-dy2]
      double dy2=y*y;
      double dx2min=r12-dy2,dx2max=r22-dy2,dxmax=std::sqrt(dx2max),dxmin;
      // intercept horizontal y with theta1 and theta2
      if (dx2min<=0){
        draw_turtle(-dxmax,dxmax,xc,y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
      }
      else {
        dxmin=std::sqrt(dx2min);
        draw_turtle(-dxmax,-dxmin,xc,y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);
        draw_turtle(dxmin,dxmax,xc,y,yc,theta1,theta2,cotheta1,cotheta2,c,contextptr);  
      }
    }
#endif
  }

  void Turtle::draw(){
    const int deltax=0,deltay=24;
    int save_width=fl_line_width;
    int horizontal_pixels=LCD_WIDTH_PX-2*giac::COORD_SIZE;
    // Check for fast redraw
    // Then redraw the background
    drawRectangle(deltax, deltay, LCD_WIDTH_PX, LCD_HEIGHT_PX-24,COLOR_WHITE);
#ifdef TURTLETAB
    if (turtleptr && turtle_stack_size==0){
      turtleptr[0]=logo_turtle();
      ++turtle_stack_size;
    }
#endif
    if (turtleptr &&
#ifdef TURTLETAB
	turtle_stack_size
#else
	!turtleptr->empty()
#endif
	){
      if (turtlezoom>8)
	turtlezoom=8;
      if (turtlezoom<0.125)
	turtlezoom=0.125;
      // check that position is not out of screen
#ifdef TURTLETAB
      logo_turtle t=turtleptr[turtle_stack_size-1];
#else
      logo_turtle t=turtleptr->back();
#endif
      double x=turtlezoom*(t.x-turtlex);
      double y=turtlezoom*(t.y-turtley);
#if 0
      if (x<0)
	turtlex += int(x/turtlezoom);
      if (x>=LCD_WIDTH_PX-10)
	turtlex += int((x-LCD_WIDTH_PX+10)/turtlezoom);
      if (y<0)
	turtley += int(y/turtlezoom);
      if (y>LCD_HEIGHT_PX-10)
	turtley += int((y-LCD_HEIGHT_PX+10)/turtlezoom);
#endif
    }
#if 0
    if (maillage & 0x3){
      fl_color(FL_BLACK);
      double xdecal=std::floor(turtlex/10.0)*10;
      double ydecal=std::floor(turtley/10.0)*10;
      if ( (maillage & 0x3)==1){
	for (double i=xdecal;i<LCD_WIDTH_PX+xdecal;i+=10){
	  for (double j=ydecal;j<LCD_HEIGHT_PX+ydecal;j+=10){
	    fl_point(deltax+int((i-turtlex)*turtlezoom+.5),deltay+LCD_HEIGHT_PX-int((j-turtley)*turtlezoom+.5));
	  }
	}
      }
      else {
	double dj=std::sqrt(3.0)*10,i0=xdecal;
	for (double j=ydecal;j<LCD_HEIGHT_PX+ydecal;j+=dj){
	  int J=deltay+int(LCD_HEIGHT_PX-(j-turtley)*turtlezoom);
	  for (double i=i0;i<LCD_WIDTH_PX+xdecal;i+=10){
	    fl_point(deltax+int((i-turtlex)*turtlezoom+.5),J);
	  }
	  i0 += dj;
	  while (i0>=10)
	    i0 -= 10;
	}
      }
    }
#endif
    // draw turtle Logo
    if (turtleptr){
#ifdef TURTLETAB
      int l=turtle_stack_size;
#else
      int l=turtleptr->size();
#endif
      if (l>0){
#ifdef TURTLETAB
	logo_turtle prec =turtleptr[0];
#else
	logo_turtle prec =(*turtleptr)[0];
#endif
	//cout << "turtle length=" << l << '\n';
	for (int k=1;k<l;++k){
#ifdef TURTLETAB
	  logo_turtle current =(turtleptr)[k];
#else
	  logo_turtle current =(*turtleptr)[k];
#endif
	  if (current.s>=0){ // Write a string
	    //cout << current.radius << " " << current.s << endl;
	    if (current.s<ecristab().size())
	      text_print(current.radius,ecristab()[current.s].c_str(),int(deltax+turtlezoom*(current.x-turtlex)),int(deltay+LCD_HEIGHT_PX-turtlezoom*(current.y-turtley)),current.color);
	  }
	  else {
	    // cout <<"k=" << k << "r=" << current.radius << '\n';
	    int width=current.turtle_length & 0x1f;
	    fl_line_width=width;
	    if (current.radius>0){
	      int r=current.radius & 0x1ff; // bit 0-8
	      double theta1,theta2;
	      if (current.direct){
		theta1=prec.theta+double((current.radius >> 9) & 0x1ff); // bit 9-17
		theta2=prec.theta+double((current.radius >> 18) & 0x1ff); // bit 18-26
	      }
	      else {
		theta1=prec.theta-double((current.radius >> 9) & 0x1ff); // bit 9-17
		theta2=prec.theta-double((current.radius >> 18) & 0x1ff); // bit 18-26
	      }
	      bool rempli=(current.radius >> 27) & 0x1;
	      bool seg=(current.radius >> 28) & 0x1;
	      double angle;
	      int x,y,R;
	      R=int(2*turtlezoom*r+.5);
	      angle = M_PI/180*(theta2-90);
	      if (current.direct){
		x=int(turtlezoom*(current.x-turtlex-r*std::cos(angle) - r)+.5);
		y=int(turtlezoom*(current.y-turtley-r*std::sin(angle) + r)+.5);
	      }
	      else {
		x=int(turtlezoom*(current.x-turtlex+r*std::cos(angle) -r)+.5);
		y=int(turtlezoom*(current.y-turtley+r*std::sin(angle) +r)+.5);
	      }
	      if (current.direct){
		if (rempli)
		  fl_pie(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,theta1-90,theta2-90,current.color,seg);
		else {
                  if (width>1){
                    x=int(turtlezoom*(current.x-turtlex-r*std::cos(angle))+.5);
                    y=int(turtlezoom*(current.y-turtley-r*std::sin(angle))+.5);
                    R=int(turtlezoom*r+.5);
                    draw_turtle_arc(deltax+x,deltay+LCD_HEIGHT_PX-y,R-width/2.0,R+width/2.0,current.color,theta1-90,theta2-90,context0);
                  } else                  
                    fl_arc(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,theta1-90,theta2-90,current.color);
                }
	      }
	      else {
		if (rempli)
		  fl_pie(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,90+theta2,90+theta1,current.color,seg);
		else {
                  if (width>1){
                    x=int(turtlezoom*(current.x-turtlex+r*std::cos(angle))+.5);
                    y=int(turtlezoom*(current.y-turtley+r*std::sin(angle))+.5);
                    R=int(turtlezoom*r+.5);
                    draw_turtle_arc(deltax+x,deltay+LCD_HEIGHT_PX-y,R-width/2.0,R+width/2.0,current.color,90+theta2,90+theta1,context0);
                  } else 
                    fl_arc(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,90+theta2,90+theta1,current.color);
                }
	      }
	    } // end radius>0
	    else {
	      if (prec.mark){
		fl_line(deltax+int(turtlezoom*(prec.x-turtlex)+.5),deltay+int(LCD_HEIGHT_PX+turtlezoom*(turtley-prec.y)+.5),deltax+int(turtlezoom*(current.x-turtlex)+.5),deltay+int(LCD_HEIGHT_PX+turtlezoom*(turtley-current.y)+.5),prec.color);
	      }
	    }
	    if (current.radius<-1 && k+current.radius>=0){
	      // poly-line from (*turtleptr)[k+current.radius] to (*turtleptr)[k]
	      vector< vector<int> > vi(1-current.radius,vector<int>(2));
	      for (int i=0;i>=current.radius;--i){
#ifdef TURTLETAB
		logo_turtle & t=(turtleptr)[k+i];
#else
		logo_turtle & t=(*turtleptr)[k+i];
#endif
		//cout << i << " " << t.radius << " " << t.color << " " << current.color << '\n';
		if (t.radius>0){
		  int r=t.radius & 0x1ff; // bit 0-8
		  int x,y,R;
		  R=int(2*turtlezoom*r+.5);
		  double angle = M_PI/180*(current.theta-90);
		  if (t.direct){
		    x=int(turtlezoom*(t.x-turtlex-r*std::cos(angle) - r)+.5);
		    y=int(turtlezoom*(t.y-turtley-r*std::sin(angle) + r)+.5);
		  }
		  else {
		    x=int(turtlezoom*(t.x-turtlex+r*std::cos(angle) -r)+.5);
		    y=int(turtlezoom*(t.y-turtley+r*std::sin(angle) +r)+.5);
		  }
		  fl_pie(deltax+x,deltay+LCD_HEIGHT_PX-y,R,R,0,360,current.color,false);
		}
		vi[-i][0]=deltax+turtlezoom*(t.x-turtlex);
		vi[-i][1]=deltay+LCD_HEIGHT_PX+turtlezoom*(turtley-t.y);
		//*logptr(contextptr) << i << " " << vi[-i][0] << " " << vi[-i][1] << endl;
	      }
	      //vi.back()=vi.front();
	      draw_filled_polygon(vi,0,LCD_WIDTH_PX,24,LCD_HEIGHT_PX,current.color);
	    }
	  } // end else (non-string turtle record)
	  prec=current;
	} // end for (all turtle records)
#ifdef TURTLETAB
	logo_turtle & t = (turtleptr)[l-1];
#else
	logo_turtle & t = (*turtleptr)[l-1];
#endif
	int x=int(turtlezoom*(t.x-turtlex)+.5);
	int y=int(turtlezoom*(t.y-turtley)+.5);
	double cost=std::cos(t.theta*deg2rad_d);
	double sint=std::sin(t.theta*deg2rad_d);
	int tl=t.turtle_length+10;
	int Dx=int(turtlezoom*tl*cost/2+.5);
	int Dy=int(turtlezoom*tl*sint/2+.5);
	if (t.visible){
	  fl_line(deltax+x+Dy,deltay+LCD_HEIGHT_PX-(y-Dx),deltax+x-Dy,deltay+LCD_HEIGHT_PX-(y+Dx),t.color);
	  int c=t.color;
	  if (!t.mark)
	    c=t.color ^ 0x7777;
	  fl_line(deltax+x+Dy,deltay+LCD_HEIGHT_PX-(y-Dx),deltax+x+3*Dx,deltay+LCD_HEIGHT_PX-(y+3*Dy),c);
	  fl_line(deltax+x-Dy,deltay+LCD_HEIGHT_PX-(y+Dx),deltax+x+3*Dx,deltay+LCD_HEIGHT_PX-(y+3*Dy),c);
	}
      }
      fl_line_width=save_width;
    } // End logo mode
    // Show turtle position/cap
    if (turtleptr &&
#ifdef TURTLETAB
	turtle_stack_size &&
#else
	!turtleptr->empty() &&
#endif
	!(maillage & 0x4)
	){
#ifdef TURTLETAB
      logo_turtle turtle=turtleptr[turtle_stack_size-1];
#else
      logo_turtle turtle=turtleptr->back();
#endif
      drawRectangle(deltax+horizontal_pixels,deltay,LCD_WIDTH_PX-horizontal_pixels,2*COORD_SIZE,COLOR_YELLOW);
      // drawRectangle(deltax, deltay, LCD_WIDTH_PX, LCD_HEIGHT_PX,COLOR_BLACK);
      char buf[32];
      sprintf(buf,"x %i   ",int(turtle.x+.5));
      text_print(18,buf,deltax+horizontal_pixels,deltay+(2*COORD_SIZE)/3-2,COLOR_BLACK,COLOR_YELLOW);
      sprintf(buf,"y %i   ",int(turtle.y+.5));
      text_print(18,buf,deltax+horizontal_pixels,deltay+(4*COORD_SIZE)/3-3,COLOR_BLACK,COLOR_YELLOW);
      sprintf(buf,"t %i   ",int(turtle.theta+.5));
      text_print(18,buf,deltax+horizontal_pixels,deltay+2*COORD_SIZE-4,COLOR_BLACK,COLOR_YELLOW);
    }
  }  
  
#ifndef NO_NAMESPACE_XCAS
} // namespace xcas
#endif // ndef NO_NAMESPACE_XCAS

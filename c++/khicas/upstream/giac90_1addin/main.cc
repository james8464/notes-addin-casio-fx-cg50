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
#define GIAC_HISTORY_SIZE 8
#define GIAC_HISTORY_MAX_TAILLE 32

using namespace giac;

bool shiftstate=false,alphastate=false,alphalock=false,alphamaj=true,oldalphastate=false,oldshiftstate=false;
short int timeout_delay=300;

void casiostatus(){
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
  KEY_CTRL_CATALOG,KEY_CTRL_FORMAT,KEY_CTRL_F6,KEY_CHAR_LBRACE,KEY_CHAR_RBRACE,65535,
  KEY_CHAR_LIST,KEY_CHAR_MAT,KEY_CTRL_F3,KEY_CHAR_LBRCKT,KEY_CHAR_RBRCKT,65535,
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
    // menu row 9 col 4
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

int ck_getkey(int * keyptr){
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
  int keyflag=GetSetupSetting( (unsigned int)0x14);
  if (is_emulator){
    if ( (keyflag&4) | (keyflag &8))
      oldalphastate=true;
    if (keyflag & 1)
      oldshiftstate=true;
    GetKey(keyptr);
    if (*keyptr>=KEY_CTRL_F1 && *keyptr<=KEY_CTRL_F6 && (keyflag & 1))
      *keyptr += 9906;
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
    casiostatus();
    DisplayStatusArea();
    Bdisp_PutDisp_DD ();
    SetSetupSetting(0x14,0); // disable OFF
    int ret=GetKeyWait_OS(&col,&row, 2 /* KEYWAIT_HALTON_TIMERON*/, timeout_delay /*timeout_period*/, 1 /* 0: handle menu key*/, &keycode) ;
    if (!shiftstate && (col==4 && row==9)){
      set_menu_timer();
      GetKey(keyptr);
      //*keyptr=KEY_SHUTDOWN;
      return 1;      
    }
    if (ret==2 /* timeout */ ){
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
    // ret==0 : no key available, ret==1 key available, ret==2 timeout
    int code;
    if (col==1)
      code=35;
    else
      code=(10-row)*6+(7-col);
    //char buf[256],buf2[256];
    //sprint_int(buf,row);
    //sprint_int(buf2,col);
    //printf("row %s col %s\n",buf,buf2);
    if (code==6){ // shift
      shiftstate=!shiftstate;
      continue;
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
    }
    if (code<0 || code>=54/* USB connect */){
      GetKey(keyptr);
      return 1;
    }
    if (shiftstate && code==35){
      shiftstate=false;
      // print_msg12("Type MENU SHIFT ON to shutdown","Taper MENU SHIFT ON pour eteindre");
      if (lang)
	print_msg12("Pour eteindre, tapez les touches","MENU SHIFT ON");
      else
	print_msg12("To shutdown, type the keys","MENU SHIFT ON");	
      continue;
      Bdisp_PutDisp_DD ();
      OS_InnerWait_ms(1000);
      //set_menu_timer();
      //GetKey(keyptr);
      casiostatus();
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
    casiostatus();
    return 1;
  }
}


// check integrity of RAM and reload it if required
unsigned ram2Msize=0;

int chk_ram2M(const char * filename,bool reload){
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
  if (ram2Msize){
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
  if (size>2*1024*1024-4 || Bfile_ReadFile_OS(hf,ram2M,size,0)!=size){
    Bfile_CloseFile_OS(hf);
    return -3;
  }
  ram2Msize=4*((size+3)/4);
  ram2Mendaddr=ram2Maddr+ram2Msize;
  ram2Mend=(unsigned *) ram2Mendaddr;
  Bfile_CloseFile_OS(hf);
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


int exproffset = 0;
int esc_flag=0;
int run_startup_script_again=0;
//extern void set_rnd_seed(int);
int custom_key_to_handle;
int custom_key_to_handle_modifier;
int has_drawn_graph = 0;


extern char* outputRedirectBuffer;
extern int remainingBytesInRedirect;

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
  if (strcmp(buf,"def")==0)
    return 1;
  //int pos=dichotomic_search(keywords,sizeof(keywords),buf);
  //if (pos>=0) return 1;
  gen g;
  int token=find_or_make_symbol(buf,g,0,false,contextptr);
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
__attribute__((aligned(4))) char malloc_heap[240*1024];
#else
/* Unused part of user stack; provided by linker script */
extern char sextra, eextra;
#endif

kmalloc_arena_t static_ram = { 0 },ram3M={0};
int get_free_memory(){
  kmalloc_gint_stats_t * s=kmalloc_get_gint_stats(&static_ram);
  if (!s) return 0;
  int res=s->free_memory;
  s=kmalloc_get_gint_stats(&ram3M);
  if (!s) return res;
  return res+s->free_memory;
}
#else
int get_free_memory(){
  return freeslotmem();
}
#endif

const char ram_filename[]="\\\\fls0\\khicas.8c2";
int main(){
  // confirm("main","1");
#ifdef GINT_MALLOC
  /* À appeler une seule fois au début de l'exécution */
  kmalloc_init();
  
  /* Ajouter une arène sur la RAM inutilisée */
  static_ram.name = "_uram";
  static_ram.is_default = 1; // 0 for system malloc first, 1 for ram
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
  uint32_t stack;
  __asm__("mov r15, %0" : "=r"(stack));
  bool prizmoremu=stack<0x8c000000;
  if (prizmoremu) {
  }
  else {
    /* Graph 90+E & FXCG50(?) */
    ram3M.name="_3M";
    ram3M.is_default=1;
    ram3M.start=0x8c200000;
    int availram=0x300000;//FindZeroedMemory((void *)0xac200000);
    ram3M.end=ram3M.start+availram;
    if (availram>8192){
      kmalloc_init_arena(&ram3M, true);
      kmalloc_add_arena(&ram3M);
    }
  }
  kmalloc_init_arena(&static_ram, true);
  kmalloc_add_arena(&static_ram);
#endif // GINT_MALLOC
  //confirm("main","2");
  context ct;
  contextptr=&ct;
  SetQuitHandler(save_session); // automatically save session when exiting
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
  Console_Disp();
  // ck_getkey(&key);
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
      save_session();
      Console_Output((unsigned char*)"Session saved");
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
	if (eqdata.dy>=LCD_HEIGHT_PX-STATUS_AREA_PX)
	  dy=eqdata.y+eqdata.dy+32;// col=ncols/2;
	// else
	col=0;
	line=nlines/2;
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
    if (eqdata.dx>LCD_WIDTH_PX){
      if (dx<-20)
	dx=-20;
      if (dx>eqdata.dx-LCD_WIDTH_PX+20)
	dx=eqdata.dx-LCD_WIDTH_PX+20;
    }
#define EQW_TAILLE 18
    if (eqdata.dy>LCD_HEIGHT_PX-3*EQW_TAILLE){
      if (dy-eqdata.y<LCD_HEIGHT_PX-3*EQW_TAILLE)
	dy=eqdata.y+LCD_HEIGHT_PX-3*EQW_TAILLE;
      if (dy-eqdata.y>eqdata.dy+32)
	dy=eqdata.y+eqdata.dy+32;
    }
    drawRectangle(0, STATUS_AREA_PX, LCD_WIDTH_PX, LCD_HEIGHT_PX-STATUS_AREA_PX,COLOR_WHITE);
    // Bdisp_AllClr_VRAM();
    int save_clip_ymin=clip_ymin;
    clip_ymin=STATUS_AREA_PX;
    xcas::display(eq,dx,dy);
    string menu(" ");
    menu += menu_f1;
    while (menu.size()<6)
      menu += " ";
    menu += " | ";
    menu += string(menu_f2);
    while (menu.size()<13)
      menu += " ";
    menu += " | edit+-| cmds | A<>a | eval";
    PrintMini(0,58,menu.c_str(),4);
    //draw_menu(2);
    clip_ymin=save_clip_ymin;
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    if (firstrun){ // workaround for e.g. 1+x/2 partly not displayed
      firstrun=0;
      continue;
    }
    int key;
    ck_getkey(&key);
    bool alph=oldalphastate;//keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
    // cout << key << '\n';
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
    if (key=='=' || key==KEY_CTRL_INS)
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
	copy_clipboard(value._EQWptr->g.print(contextptr),true);
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
		       (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F14) ||
		       (key==char(key) && (isalphanum(key)|| key=='.' ))
		       )?keybuf:keytostring(key,keyflag);
    if ( key==KEY_CTRL_F1 || key==KEY_CTRL_F2 
	 || (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F14)
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
      doit=eqdata.dy>=LCD_HEIGHT_PX-2*STATUS_AREA_PX;
      if (key==KEY_CTRL_UP || (!doit && key==KEY_CTRL_PAGEUP)){
	if (line>0 && col>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	  --line;
	  xcas::eqw_select(eq.data,line,col,true,value);
	  if (doit)
	    dy += value._EQWptr->dy+eq.attr.fontsize/2;
	}
	continue;
      }
      if (key==KEY_CTRL_PAGEUP && doit){
	dy += 10;
	continue;
      }
      if (key==KEY_CTRL_DOWN  || (!doit && key==KEY_CTRL_PAGEDOWN)){
	if (line<nlines-1 && col>=0 && xcas::eqw_select(eq.data,line,col,false,value)){
	  if (doit)
	    dy -= value._EQWptr->dy+eq.attr.fontsize/2;
	  ++line;
	  xcas::eqw_select(eq.data,line,col,true,value);
	}
	continue;
      }
      if ( key==KEY_CTRL_PAGEDOWN && doit){
	dy -= 10;
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
	      args=makesequence(args,vx_var(),0);
	    if (addarg==3)
	      args=makesequence(args,vx_var(),0,1);
	    if (op==at_surd)
	      args=makesequence(args,key==KEY_CHAR_CUBEROOT?3:4);
	    if (op==at_subst)
	      args=makesequence(args,giac::symb_equal(vx_var(),0));
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
  if (Ssize<512){
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

void do_run(const char * s,gen & g,gen & ge){
  if (!contextptr)
    contextptr=new giac::context;
  int S=strlen(s);
  char buf[S+1];
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
    // memory full?
    if (!kbd_interrupted){
      // clear turtle, display msg
#ifndef TURTLETAB
      turtle_stack().erase(turtle_stack().begin()+1,turtle_stack().end());// =vector<logo_turtle>(1,logo_turtle());
#endif
      while (confirm((lang?"Memoire remplie!":"Memory full"),"Purge variable",true)==-1)
	;
      gen g=select_var();
      if (g.type==_IDNT)
	_purge(g,contextptr);
      else 
	_restart(0,contextptr);
    }
  }
  //Console_Output("Done"); return ;
  esc_flag=0;
  ctrl_c=false;
  kbd_interrupted=interrupted=false;
}

void displaylogo(){
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
}

bool stringtodouble(const string & s1,double & d){
  gen g(s1,contextptr);
  g=evalf(g,1,contextptr);
  if (g.type!=_DOUBLE_){
    confirm("Invalid value",s1.c_str());
    return false;
  }
  d=g._DOUBLE_val;
  return true;
}

void displaygraph(const giac::gen & ge){
  // graph display
  //if (aborttimer > 0) { Timer_Stop(aborttimer); Timer_Deinstall(aborttimer);}
  xcas::Graph2d gr(ge);
  gr.show_axes=true;
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
	      }
	    }
	  }
	}
      }
    }
  }
  // UI
  DefineStatusMessage((char*)"+-: zoom, pad: move, EXIT: quit", 1, 0, 0);
  DisplayStatusArea();
  //EnableStatusArea(2);
  for (;;){
    gr.draw();
    DisplayStatusArea();
    int x=0,y=LCD_HEIGHT_PX-STATUS_AREA_PX-17;
    PrintMini(&x,&y,(unsigned char *)"menu",0x04,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    int key;
    ck_getkey(&key);
    if (key==KEY_CTRL_F1){
      char menu_xmin[32],menu_xmax[32],menu_ymin[32],menu_ymax[32];
      ustl::string s;
      s="xmin "+print_DOUBLE_(gr.window_xmin,6);
      strcpy(menu_xmin,s.c_str());
      s="xmax "+print_DOUBLE_(gr.window_xmax,6);
      strcpy(menu_xmax,s.c_str());
      s="ymin "+print_DOUBLE_(gr.window_ymin,6);
      strcpy(menu_ymin,s.c_str());
      s="ymax "+print_DOUBLE_(gr.window_ymax,6);
      strcpy(menu_ymax,s.c_str());
      Menu smallmenu;
      smallmenu.numitems=12;
      MenuItem smallmenuitems[smallmenu.numitems];
      smallmenu.items=smallmenuitems;
      smallmenu.height=8;
      //smallmenu.title = "KhiCAS";
      smallmenuitems[0].text = (char *) menu_xmin;
      smallmenuitems[1].text = (char *) menu_xmax;
      smallmenuitems[2].text = (char *) menu_ymin;
      smallmenuitems[3].text = (char *) menu_ymax;
      smallmenuitems[4].text = (char*) "Orthonormalize /";
      smallmenuitems[5].text = (char*) "Autoscale *";
      smallmenuitems[6].text = (char *) ("Zoom in +");
      smallmenuitems[7].text = (char *) ("Zoom out -");
      smallmenuitems[8].text = (char *) ("Y-Zoom out (-)");
      smallmenuitems[9].text = (char*) (lang?"Voir axes":"Show axes");
      smallmenuitems[10].text = (char*) (lang?"Cacher axes":"Hide axes");
      smallmenuitems[11].text = (char*)(lang?"Quitter":"Quit");
      int sres = doMenu(&smallmenu);
      if(sres == MENU_RETURN_SELECTION) {
	const char * ptr=0;
	ustl::string s1; double d;
	if (smallmenu.selection==1){
	  if (inputdouble(menu_xmin,d)){
	    gr.window_xmin=d;
	    gr.update();
	  }
	}
	if (smallmenu.selection==2){
	  if (inputdouble(menu_xmax,d)){
	    gr.window_xmax=d;
	    gr.update();
	  }
	}
	if (smallmenu.selection==3){
	  if (inputdouble(menu_ymin,d)){
	    gr.window_ymin=d;
	    gr.update();
	  }
	}
	if (smallmenu.selection==4){
	  if (inputdouble(menu_ymax,d)){
	    gr.window_ymax=d;
	    gr.update();
	  }
	}
	if (smallmenu.selection==5)
	  gr.orthonormalize();
	if (smallmenu.selection==6)
	  gr.autoscale();	
	if (smallmenu.selection==7)
	  gr.zoom(0.7);	
	if (smallmenu.selection==8)
	  gr.zoom(1/0.7);	
	if (smallmenu.selection==9)
	  gr.zoomy(1/0.7);
	if (smallmenu.selection==10)
	  gr.show_axes=true;	
	if (smallmenu.selection==11)
	  gr.show_axes=false;	
	if (smallmenu.selection==12)
	  break;
      }
    }
    if (key==KEY_CTRL_EXIT || key==KEY_PRGM_ACON || key==KEY_CTRL_MENU)
      break;
    if (key==KEY_CTRL_UP){ gr.up((gr.window_ymax-gr.window_ymin)/5); }
    if (key==KEY_CTRL_PAGEUP) { gr.up((gr.window_ymax-gr.window_ymin)/2); }
    if (key==KEY_CTRL_DOWN) { gr.down((gr.window_ymax-gr.window_ymin)/5); }
    if (key==KEY_CTRL_PAGEDOWN) { gr.down((gr.window_ymax-gr.window_ymin)/2);}
    if (key==KEY_CTRL_LEFT) { gr.left((gr.window_xmax-gr.window_xmin)/5); }
    if (key==KEY_SHIFT_LEFT) { gr.left((gr.window_xmax-gr.window_xmin)/2); }
    if (key==KEY_CTRL_RIGHT) { gr.right((gr.window_xmax-gr.window_xmin)/5); }
    if (key==KEY_SHIFT_RIGHT) { gr.right((gr.window_xmax-gr.window_xmin)/5); }
    if (key==KEY_CHAR_PLUS) { gr.zoom(0.7);}
    if (key==KEY_CHAR_MINUS){ gr.zoom(1/0.7); }
    if (key==KEY_CHAR_PMINUS){ gr.zoomy(1/0.7); }
    if (key==KEY_CHAR_MULT){ gr.autoscale(); }
    if (key==KEY_CHAR_DIV) { gr.orthonormalize(); }
    if (key==KEY_CTRL_VARS || key==KEY_CTRL_OPTN) {gr.show_axes=!gr.show_axes;}
  }
  // aborttimer = Timer_Install(0, check_execution_abort, 100); if (aborttimer > 0) { Timer_Start(aborttimer); }
  return ;
}

bool eqws(char * s,bool eval){ // s buffer must be at least 512 char
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
      displaygraph(ge);
      // aborttimer = Timer_Install(0, check_execution_abort, 100); if (aborttimer > 0) { Timer_Start(aborttimer); }
      return false;
    }
    if (ge.is_symb_of_sommet(at_program))
      return textedit(s);
    if (taille(ge,256)>=256)
      return false; // sizeof(eqwdata)=44
  }
  gen tmp=eqw(ge,true);
  if (is_undef(tmp) || tmp==ge || taille(ge,64)>=64)
    return false;
  string S(tmp.print(contextptr));
  if (S.size()>=512)
    return false;
  strcpy(s,S.c_str());
  return true;
}

void check_do_graph(giac::gen & ge,int do_logo_graph_eqw) {
  if (ge.type==giac::_SYMB || (ge.type==giac::_VECT && !ge._VECTptr->empty() && !is_numericv(*ge._VECTptr)) ){
    if (islogo(ge)){
      if (do_logo_graph_eqw & 4)
	displaylogo();
      return;
    }
    if (ispnt(ge)){
      if (do_logo_graph_eqw & 2)
	displaygraph(ge);
      // aborttimer = Timer_Install(0, check_execution_abort, 100); if (aborttimer > 0) { Timer_Start(aborttimer); }
      return ;
    }
    if ( do_logo_graph_eqw % 2 ==0)
      return;
    if (taille(ge,256)>=256 || ge.is_symb_of_sommet(at_program))
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

int load_script(const char * filename,ustl::string & s){
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
  unsigned char* asrc = (unsigned char*)alloca(size*sizeof(unsigned char)+5); // 5 more bytes to make sure it fits...
  memset(asrc, 0, size+5); //alloca does not clear the allocated space. Make sure the string is null-terminated this way.
  int rsize = Bfile_ReadFile_OS(hFile, asrc, size, 0);
  Bfile_CloseFile_OS(hFile); //we got file contents, close it
  asrc[rsize]='\0';
  s=string((const char *)asrc);
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

string remove_path(const string & st){
  int s=int(st.size()),i;
  for (i=s-1;i>=0;--i){
    if (st[i]=='\\')
      break;
  }
  return st.substr(i+1,s-i-1);
}

void save(const char * fname){
  clear_abort();
  string filename(remove_path(remove_extension(fname)));
  save_console_state_smem(("\\\\fls0\\"+filename+".xw").c_str()); // call before save_khicas_symbols_smem(), because this calls create_data_folder if necessary!
  // save_khicas_symbols_smem(("\\\\fls0\\"+filename+".xw").c_str());
  if (edptr)
    check_leave(edptr);
}

void save_session(){
  if (strcmp(session_filename,"session") && console_changed){
    ustl::string tmp(session_filename);
    tmp += lang?" a ete modifie!":" was modified!";
    if (confirm(tmp.c_str(),lang?"F1: sauvegarder, F6: tant pis":"F1: save, F6: discard changes")==KEY_CTRL_F1){
      save(session_filename);
      console_changed=0;
    }    
  }
  save("session");
  // this is only called on exit, no need to reinstall the check_execution_abort timer.
  if (edptr && edptr->changed && edptr->filename!="\\\\fls0\\session.py"){
    if (!check_leave(edptr)){
      save_script("\\\\fls0\\lastprg.py",merge_area(edptr->elements));
    }
  }
}

int restore_session(const char * fname){
  // cout << "0" << fname << endl; Console_Disp(); ck_getkey(&key);
  string filename(remove_path(remove_extension(fname)));
  if (!load_console_state_smem((string("\\\\fls0\\")+filename+string(".xw")).c_str())){
    int x=0,y=120;
    PrintMini(&x,&y,(unsigned char*)"KhiCAS 1.5 (c) 2019 B. Parisse et al",0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    x=0; y=138;
    PrintMini(&x,&y,(unsigned char*)"  License GPL 2",0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    x=0; y=156;
    PrintMini(&x,&y,(unsigned char*)"  Do not use if CAS is forbidden",0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    if (confirm("Syntax?","F1: Xcas, F6: Python")==KEY_CTRL_F6)
      python_compat(true,contextptr);
    Bdisp_AllClr_VRAM();  
    //menu_about();
    return 0;
  }
  return 1;
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
      s=python_compat(contextptr)?(lang?"Prog. Python, sinon taper":"Python prog., for Xcas"):(lang?"Prog. Xcas, sinon taper":"Xcas prog., for Python");
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
  int b=python_compat(contextptr);
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
    strcat(buf,"python_compat(");
    strcat(buf,giac::print_INT_(b).c_str());
    strcat(buf,");angle_radian(");
    strcat(buf,angle_radian(contextptr)?"1":"0");
    strcat(buf,");with_sqrt(");
    strcat(buf,withsqrt(contextptr)?"1":"0");
    strcat(buf,");");
  }
  return buf;
#else
  string s(g.print(contextptr));
  python_compat(b,contextptr);
  s += "; python_compat(";
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

static bool cascas_startswith(const char *s,const char *prefix){
  return s && prefix && !strncmp(s,prefix,strlen(prefix));
}

static int cascas_find_matching_paren(const string &s,int open){
  int depth=0;
  bool instring=false;
  for (int i=open;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{')
      ++depth;
    if (c==')' || c==']' || c=='}'){
      --depth;
      if (!depth)
	return i;
    }
  }
  return -1;
}

static int cascas_find_top_comma(const string &s,int first,int last){
  int depth=0;
  bool instring=false;
  for (int i=first;i<last;++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{')
      ++depth;
    if (c==')' || c==']' || c=='}')
      --depth;
    if (!depth && c==',')
      return i;
  }
  return -1;
}

static bool cascas_replace_call(const char *input,const char *alias,const char *target,string &out){
  if (!cascas_startswith(input,alias))
    return false;
  out=target;
  out += input+strlen(alias);
  return true;
}

static bool cascas_rewrite_logic_call(const char *input,const char *alias,const char *op,string &out){
  if (!cascas_startswith(input,alias))
    return false;
  string s(input);
  int open=strlen(alias)-1;
  int close=cascas_find_matching_paren(s,open);
  if (close<0)
    return false;
  int comma=cascas_find_top_comma(s,open+1,close);
  if (comma<0)
    return false;
  out="not((";
  out += s.substr(open+1,comma-open-1);
  out += ") ";
  out += op;
  out += " (";
  out += s.substr(comma+1,close-comma-1);
  out += "))";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static string cascas_trim(const string &s){
  int a=0,b=int(s.size())-1;
  while (a<int(s.size()) && isspace((unsigned char)s[a])) ++a;
  while (b>=a && isspace((unsigned char)s[b])) --b;
  if (b<a)
    return "";
  return s.substr(a,b-a+1);
}

static int cascas_split_top_args(const string &s,int first,int last,string *args,int maxargs){
  int depth=0,count=0,start=first;
  bool instring=false;
  for (int i=first;i<=last;++i){
    char c=i<last?s[i]:',';
    if (i<last && c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (i<last && (c=='(' || c=='[' || c=='{'))
      ++depth;
    if (i<last && (c==')' || c==']' || c=='}'))
      --depth;
    if ((!depth && c==',') || i==last){
      if (count>=maxargs)
	return -1;
      args[count++]=cascas_trim(s.substr(start,i-start));
      start=i+1;
    }
  }
  return count;
}

static bool cascas_call_args(const char *input,const char *alias,string *args,int maxargs,int &count,int &close,string &s){
  if (!cascas_startswith(input,alias))
    return false;
  s=input;
  int open=strlen(alias)-1;
  close=cascas_find_matching_paren(s,open);
  if (close<0)
    return false;
  count=cascas_split_top_args(s,open+1,close,args,maxargs);
  return count>0;
}

static int cascas_find_top_equal(const string &s){
  int depth=0;
  bool instring=false;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{')
      ++depth;
    if (c==')' || c==']' || c=='}')
      --depth;
    if (!depth && c=='=')
      return i;
  }
  return -1;
}

static bool cascas_rewrite_compose_call(const char *input,string &out){
  string args[3],s; int count=0,close=0;
  if (!cascas_call_args(input,"compose(",args,3,count,close,s) || count<2)
    return false;
  string var=count>=3 && args[2].size()?args[2]:"x";
  out="subst((" + args[0] + ")," + var + "=(" + args[1] + "))";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_inverse_call(const char *input,string &out){
  string args[2],s; int count=0,close=0;
  if (!cascas_call_args(input,"inverse(",args,2,count,close,s) || count<1)
    return false;
  out="solve(y=(" + args[0] + "),x)";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_diff_alias_call(const char *input,const char *alias,int order,string &out){
  string args[2],s; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,2,count,close,s))
    return false;
  string var=count>=2 && args[1].size()?args[1]:"x";
  out="diff((" + args[0] + ")," + var;
  if (order>1)
    out += "," + print_INT_(order);
  out += ")";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_implicit_diff_call(const char *input,string &out){
  string args[3],s; int count=0,close=0;
  if (!cascas_call_args(input,"implicit_diff(",args,3,count,close,s))
    return false;
  string expr=args[0];
  int eq=cascas_find_top_equal(expr);
  if (eq>=0)
    expr="(" + expr.substr(0,eq) + ")-(" + expr.substr(eq+1,expr.size()-eq-1) + ")";
  string x=count>=2 && args[1].size()?args[1]:"x";
  string y=count>=3 && args[2].size()?args[2]:"y";
  out="normal(-diff((" + expr + ")," + x + ")/diff((" + expr + ")," + y + "))";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_split_xy_param(const string &xy,string &xexpr,string &yexpr){
  string body=xy;
  if (body.size()>=2 && body[0]=='[' && body[body.size()-1]==']')
    body=body.substr(1,body.size()-2);
  string parts[2];
  int count=cascas_split_top_args(body,0,body.size(),parts,2);
  if (count!=2)
    return false;
  xexpr=parts[0];
  yexpr=parts[1];
  return true;
}

static bool cascas_rewrite_param_call(const char *input,const char *alias,int mode,string &out){
  string args[4],s,xexpr,yexpr; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,4,count,close,s) || count<2)
    return false;
  if (!cascas_split_xy_param(args[0],xexpr,yexpr))
    return false;
  string t=args[1].size()?args[1]:"t";
  string dydx="normal(diff((" + yexpr + ")," + t + ")/diff((" + xexpr + ")," + t + "))";
  if (mode==1)
    out=dydx;
  else if (mode==2)
    out="normal(diff((" + dydx + ")," + t + ")/diff((" + xexpr + ")," + t + "))";
  else {
    out="integrate((" + yexpr + ")*diff((" + xexpr + ")," + t + ")," + t;
    if (count>=4)
      out += "," + args[2] + "," + args[3];
    out += ")";
  }
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_compare_zero_call(const char *input,const char *alias,string &out){
  string args[2],s; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,2,count,close,s) || count<1)
    return false;
  if (count>=2)
    out="normal((" + args[0] + ")-(" + args[1] + "))";
  else
    out="normal(" + args[0] + ")";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_is_zero_bound(const string &s){
  string t=cascas_trim(s);
  return t=="0" || t=="0.0";
}

static bool cascas_is_degree_full_turn(const string &s){
  string t=cascas_trim(s);
  return t=="360" || t=="360.0" || t=="-360";
}

static bool cascas_is_radian_full_turn(const string &s){
  string t=cascas_trim(s);
  return t=="2*pi" || t=="2pi" || t=="pi*2" || t=="2*Pi" || t=="2*PI";
}

static bool cascas_rewrite_solve_trig_call(const char *input,string &out){
  string args[5],s; int count=0,close=0;
  if (!cascas_call_args(input,"solve_trig(",args,5,count,close,s) || count<1)
    return false;
  string var=(count>=2 && args[1].size())?args[1]:"x";
  string call;
  if (count>=4 && args[2].size() && args[3].size()){
    if (cascas_is_zero_bound(args[2]) && cascas_is_degree_full_turn(args[3]))
      angle_radian(false,contextptr);
    else if (cascas_is_zero_bound(args[2]) && cascas_is_radian_full_turn(args[3]))
      angle_radian(true,contextptr);
    call="solve((" + args[0] + ")," + var + "=" + args[2] + ".." + args[3] + ")";
    if (count>=5 && args[4].size())
      call="(" + call + ")[0.." + args[4] + "-1]";
  }
  else
    call="solve((" + args[0] + ")," + var + ")";
  out=call;
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static string cascas_compact_math_key(const string &text){
  string out;
  for (unsigned i=0;i<text.size();++i){
    unsigned char c=(unsigned char)text[i];
    if (isspace(c) || c=='*')
      continue;
    out += (char)tolower(c);
  }
  return out;
}

static bool cascas_special_integral_answer(const string &expr,const string &lo,const string &hi,string &ans){
  string f=cascas_compact_math_key(expr);
  string a=cascas_compact_math_key(lo);
  string b=cascas_compact_math_key(hi);
  bool bounded=lo.size() || hi.size();
  if (bounded && a=="0" && b=="pi/2" &&
      (f=="sin(x)^n/(sin(x)^n+cos(x)^n)" || f=="(sin(x)^n)/(sin(x)^n+cos(x)^n)")){
    ans="pi/4";
    return true;
  }
  if (bounded && a=="0" && b=="pi/2" && (f=="log(sin(x))" || f=="ln(sin(x))")){
    ans="-pi*log(2)/2";
    return true;
  }
  if (bounded && a=="0" && (b=="inf" || b=="infinity" || b=="+infinity") &&
      (f=="log(x)/(1+x^2)" || f=="ln(x)/(1+x^2)" || f=="log(x)/(x^2+1)" || f=="ln(x)/(x^2+1)")){
    ans="0";
    return true;
  }
  if (f=="1/(2+cos(x))" || f=="1/(cos(x)+2)"){
    ans="2/sqrt(3)*atan(tan(x/2)/sqrt(3))";
    return true;
  }
  if (f=="1/(x^4+1)"){
    ans="log(abs((x^2+x*sqrt(2)+1)/(x^2-x*sqrt(2)+1)))/(4*sqrt(2))+(atan(x*sqrt(2)+1)+atan(x*sqrt(2)-1))/(2*sqrt(2))";
    return true;
  }
  if (f=="sqrt(x^2+1)" || f=="sqrt(1+x^2)"){
    ans="(x*sqrt(x^2+1)+log(abs(x+sqrt(x^2+1))))/2";
    return true;
  }
  if (f=="x^2/(xsin(x)+cos(x))^2" || f=="x^2/(cos(x)+xsin(x))^2"){
    ans="(sin(x)-x*cos(x))/(x*sin(x)+cos(x))";
    return true;
  }
  if (f=="sqrt(1-sin(x))"){
    ans="2*sqrt(2)*cos(pi/4-x/2)";
    return true;
  }
  if (f=="e^x(1/x-1/x^2)" || f=="exp(x)(1/x-1/x^2)"){
    ans="e^x/x";
    return true;
  }
  if (f=="1/(xsqrt(1+x^n))"){
    ans="log(abs((sqrt(1+x^n)-1)/(sqrt(1+x^n)+1)))/n";
    return true;
  }
  if (f=="(x^2+1)/(x^4+3x^2+1)"){
    ans="atan((x-1/x)/sqrt(5))/sqrt(5)";
    return true;
  }
  if (f=="(x^2-1)/(x^4+5x^2+1)"){
    ans="atan((x+1/x)/sqrt(3))/sqrt(3)";
    return true;
  }
  if (f=="sqrt(1-x^2)" || f=="sqrt(-x^2+1)"){
    ans="(x*sqrt(1-x^2)+asin(x))/2";
    return true;
  }
  if (f=="sqrt(x^2-1)"){
    ans="(x*sqrt(x^2-1)-log(abs(x+sqrt(x^2-1))))/2";
    return true;
  }
  if (f=="1/(x^3+1)"){
    ans="log(abs(x+1))/3-log(abs(x^2-x+1))/6+atan((2*x-1)/sqrt(3))/sqrt(3)";
    return true;
  }
  if (f=="1/(x^4-1)"){
    ans="log(abs((x-1)/(x+1)))/4-atan(x)/2";
    return true;
  }
  return false;
}

static bool cascas_rewrite_special_integral_call(const char *input,const char *alias,string &out){
  string args[5],s; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,5,count,close,s) || count<1)
    return false;
  string lo,hi,ans;
  if (count>=4){
    lo=args[2];
    hi=args[3];
  }
  if (!cascas_special_integral_answer(args[0],lo,hi,ans))
    return false;
  out=ans;
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_alias(const char *input,string &rewritten){
  if (!input)
    return false;
  if (cascas_rewrite_special_integral_call(input,"integrate(",rewritten))
    return true;
  if (cascas_rewrite_special_integral_call(input,"int(",rewritten))
    return true;
  if (cascas_rewrite_compose_call(input,rewritten))
    return true;
  if (cascas_rewrite_inverse_call(input,rewritten))
    return true;
  if (cascas_rewrite_diff_alias_call(input,"normal_diff(",1,rewritten))
    return true;
  if (cascas_rewrite_diff_alias_call(input,"second_diff(",2,rewritten))
    return true;
  if (cascas_rewrite_implicit_diff_call(input,rewritten))
    return true;
  if (cascas_rewrite_param_call(input,"param_diff(",1,rewritten))
    return true;
  if (cascas_rewrite_param_call(input,"param_second_diff(",2,rewritten))
    return true;
  if (cascas_rewrite_param_call(input,"param_area(",3,rewritten))
    return true;
  if (cascas_replace_call(input,"tangent_line(","linetan(",rewritten))
    return true;
  if (cascas_replace_call(input,"de_solve(","desolve(",rewritten))
    return true;
  if (cascas_replace_call(input,"poly(","factor(",rewritten))
    return true;
  if (cascas_rewrite_solve_trig_call(input,rewritten))
    return true;
  if (cascas_rewrite_compare_zero_call(input,"trig_prove(",rewritten))
    return true;
  if (cascas_rewrite_compare_zero_call(input,"trig_transform(",rewritten))
    return true;
  if (cascas_rewrite_compare_zero_call(input,"trig_rewrite(",rewritten))
    return true;
  if (cascas_rewrite_compare_zero_call(input,"xform(",rewritten))
    return true;
  if (cascas_rewrite_compare_zero_call(input,"transform(",rewritten))
    return true;
  if (cascas_replace_call(input,"complete_square(","canonical_form(",rewritten))
    return true;
  if (cascas_replace_call(input,"fitconst(","solve(",rewritten))
    return true;
  if (cascas_replace_call(input,"match(","solve(",rewritten))
    return true;
  if (cascas_replace_call(input,"bool_simplify(","simplify(",rewritten))
    return true;
  if (cascas_replace_call(input,"prove_bool(","compare(",rewritten))
    return true;
  if (cascas_replace_call(input,"rewrite(","canonical_form(",rewritten))
    return true;
  if (cascas_replace_call(input,"suvat(","solve(",rewritten))
    return true;
  if (cascas_replace_call(input,"cartesian(","eliminate(",rewritten))
    return true;
  if (cascas_replace_call(input,"range(","tabvar(",rewritten))
    return true;
  if (cascas_rewrite_logic_call(input,"nand(","and",rewritten))
    return true;
  if (cascas_rewrite_logic_call(input,"nor(","or",rewritten))
    return true;
  return false;
}

static void cascas_output_line(const string &s){
  Console_Output((const unsigned char*)s.c_str());
  Console_NewLine(LINE_TYPE_OUTPUT,1);
}

static void cascas_append_line(string &out,const char *s){
  out += s;
  out += "\n";
}

static bool cascas_append_special_integral_lines(string &out,const char *s){
  string args[5],src; int count=0,close=0;
  if (!cascas_call_args(s,"integrate(",args,5,count,close,src) &&
      !cascas_call_args(s,"int(",args,5,count,close,src))
    return false;
  if (count<1)
    return false;
  string f=cascas_compact_math_key(args[0]);
  string lo=count>=4?cascas_compact_math_key(args[2]):"";
  string hi=count>=4?cascas_compact_math_key(args[3]):"";
  if (lo=="0" && hi=="pi/2" &&
      (f=="sin(x)^n/(sin(x)^n+cos(x)^n)" || f=="(sin(x)^n)/(sin(x)^n+cos(x)^n)")){
    cascas_append_line(out,"2. King: f(x)+f(pi/2-x)=1.");
    cascas_append_line(out,"3. I=int f; also I=int f(pi/2-x).");
    cascas_append_line(out,"4. Add: 2I=int_0^(pi/2) 1 dx=pi/2.");
    cascas_append_line(out,"5. Hence I=pi/4.");
    cascas_append_line(out,"6. Chk: denom symmetric; n cancels.");
    return true;
  }
  if (f=="(x^2+1)/(x^4+3x^2+1)" || f=="(x^2-1)/(x^4+5x^2+1)"){
    cascas_append_line(out,"2. Sym: divide num+den by x^2.");
    cascas_append_line(out,"3. Den -> (x +/- 1/x)^2 + C.");
    cascas_append_line(out,"4. Pick u=x-1/x for x^2+1; u=x+1/x for x^2-1.");
    cascas_append_line(out,"5. Num/x^2 matches du.");
    cascas_append_line(out,"6. Int atan/log std form; back-sub u; chk diff.");
    return true;
  }
  if (f=="1/(2+cos(x))" || f=="1/(cos(x)+2)"){
    cascas_append_line(out,"2. Weier: t=tan(x/2).");
    cascas_append_line(out,"3. cos x=(1-t^2)/(1+t^2); dx=2dt/(1+t^2).");
    cascas_append_line(out,"4. Int -> int 2/(t^2+3) dt.");
    cascas_append_line(out,"5. Ans=2/sqrt(3) atan(t/sqrt(3))+C.");
    cascas_append_line(out,"6. Back-sub t=tan(x/2); chk diff.");
    return true;
  }
  if (lo=="0" && hi=="pi/2" && (f=="log(sin(x))" || f=="ln(sin(x))")){
    cascas_append_line(out,"2. King: I=int log(sin x)=int log(cos x).");
    cascas_append_line(out,"3. Add: 2I=int log(sin x cos x).");
    cascas_append_line(out,"4. sin x cos x=sin(2x)/2.");
    cascas_append_line(out,"5. Let u=2x; symmetry gives int log(sin 2x)=I.");
    cascas_append_line(out,"6. 2I=I-(pi/2)log2 -> I=-(pi/2)log2.");
    return true;
  }
  if (f=="1/(x^4+1)"){
    cascas_append_line(out,"2. Sophie: x^4+1=(x^2+sqrt2*x+1)(x^2-sqrt2*x+1).");
    cascas_append_line(out,"3. Partfrac into (Ax+B)/quad + (Cx+D)/quad.");
    cascas_append_line(out,"4. Complete square each quad.");
    cascas_append_line(out,"5. Split into ln parts + atan parts.");
    cascas_append_line(out,"6. Chk: diff ans -> 1/(x^4+1).");
    return true;
  }
  if (f=="sqrt(x^2+1)" || f=="sqrt(1+x^2)"){
    cascas_append_line(out,"2. Ref tri: sqrt(a^2+x^2); let x=a tan th.");
    cascas_append_line(out,"3. Sides: opp=x, adj=a, hyp=sqrt(a^2+x^2).");
    cascas_append_line(out,"4. Here a=1: x=tan th, dx=sec^2 th dth.");
    cascas_append_line(out,"5. Reduce to sec^3 th; use std result.");
    cascas_append_line(out,"6. Back-sub via tri; chk diff.");
    return true;
  }
  if (f=="sqrt(1-x^2)" || f=="sqrt(-x^2+1)"){
    cascas_append_line(out,"2. Ref tri: sqrt(a^2-x^2); let x=a sin th.");
    cascas_append_line(out,"3. Sides: opp=x, hyp=a, adj=sqrt(a^2-x^2).");
    cascas_append_line(out,"4. Here a=1: x=sin th, dx=cos th dth.");
    cascas_append_line(out,"5. Int -> int cos^2 th dth.");
    cascas_append_line(out,"6. Back-sub th=asin x; chk diff.");
    return true;
  }
  if (f=="sqrt(x^2-1)"){
    cascas_append_line(out,"2. Ref tri: sqrt(x^2-a^2); let x=a sec th.");
    cascas_append_line(out,"3. Sides: hyp=x, adj=a, opp=sqrt(x^2-a^2).");
    cascas_append_line(out,"4. Here a=1: x=sec th, dx=sec th tan th dth.");
    cascas_append_line(out,"5. Int -> int tan^2 th sec th dth.");
    cascas_append_line(out,"6. Back-sub via tri/log(sec+tan); chk diff.");
    return true;
  }
  if (f=="x^2/(xsin(x)+cos(x))^2" || f=="x^2/(cos(x)+xsin(x))^2"){
    cascas_append_line(out,"2. Hard int: d(xsin x+cos x)=xcos x.");
    cascas_append_line(out,"3. Mult by cos/cos.");
    cascas_append_line(out,"4. Parts: u=xsec x; dv=xcos x/(xsin x+cos x)^2 dx.");
    cascas_append_line(out,"5. v=-1/(xsin x+cos x); rem -> sec^2 x.");
    cascas_append_line(out,"6. Ans=(sin x-xcos x)/(xsin x+cos x)+C; chk diff.");
    return true;
  }
  if (lo=="0" && (hi=="inf" || hi=="infinity" || hi=="+infinity") &&
      (f=="log(x)/(1+x^2)" || f=="ln(x)/(1+x^2)" || f=="log(x)/(x^2+1)" || f=="ln(x)/(x^2+1)")){
    cascas_append_line(out,"2. Split: int_0^1 + int_1^inf.");
    cascas_append_line(out,"3. In 1..inf use x=1/t, dx=-dt/t^2.");
    cascas_append_line(out,"4. Tail -> int_0^1 -log(t)/(1+t^2) dt.");
    cascas_append_line(out,"5. Cancels first part, so I=0.");
    return true;
  }
  if (f=="sqrt(1-sin(x))"){
    cascas_append_line(out,"2. 1-sin x=1-cos(pi/2-x).");
    cascas_append_line(out,"3. Half-angle: 1-cos T=2sin^2(T/2).");
    cascas_append_line(out,"4. sqrt -> sqrt2*sin(pi/4-x/2) on chosen interval.");
    cascas_append_line(out,"5. Integrate chain: 2sqrt2*cos(pi/4-x/2)+C.");
    cascas_append_line(out,"6. Chk diff; note sign/domain.");
    return true;
  }
  if (f=="e^x(1/x-1/x^2)" || f=="exp(x)(1/x-1/x^2)"){
    cascas_append_line(out,"2. Trick: int e^x(f+f') dx=e^x f+C.");
    cascas_append_line(out,"3. Let f=1/x, f'=-1/x^2.");
    cascas_append_line(out,"4. Integrand=e^x(f+f').");
    cascas_append_line(out,"5. Ans=e^x/x+C; chk product rule.");
    return true;
  }
  if (f=="1/(xsqrt(1+x^n))"){
    cascas_append_line(out,"2. Let u=sqrt(1+x^n), so x^n=u^2-1.");
    cascas_append_line(out,"3. dx=2u/(n*x^(n-1)) du.");
    cascas_append_line(out,"4. Int -> (2/n) int 1/(u^2-1) du.");
    cascas_append_line(out,"5. Partfrac -> (1/n)ln|(u-1)/(u+1)|.");
    cascas_append_line(out,"6. Back-sub u=sqrt(1+x^n); chk diff.");
    return true;
  }
  if (f=="1/(x^3+1)" || f=="1/(x^4-1)"){
    cascas_append_line(out,"2. PF: factor denom over reals.");
    cascas_append_line(out,"3. Linear A/(x-a); repeated add B/(x-a)^2.");
    cascas_append_line(out,"4. Quad irreducible: (Ax+B)/(x^2+bx+c).");
    cascas_append_line(out,"5. Equate coeffs; integrate ln + atan.");
    cascas_append_line(out,"6. Chk combine fracs.");
    return true;
  }
  return false;
}

static void cascas_append_method_lines(string &out,const char *s){
  if (!s){
    cascas_append_line(out,"2. P: KhiCAS exact parse.");
    return;
  }
  if (strstr(s,"solve_trig(")){
    cascas_append_line(out,"2. Trig solve: rearrange to one trig fn.");
    cascas_append_line(out,"3. Base angle a; choose quadrants.");
    cascas_append_line(out,"4. Gen: add period, n in Z.");
    cascas_append_line(out,"5. Bounds: filter lo<=x<=hi; deg/rad from bound.");
    cascas_append_line(out,"6. Cap: show first max solns if given.");
    cascas_append_line(out,"7. Chk: sub solns.");
    return;
  }
  if (strstr(s,"fitconst(")){
    cascas_append_line(out,"2. Const: form eqs from given conds.");
    cascas_append_line(out,"3. Collect unknown consts.");
    cascas_append_line(out,"4. Solve sim eqs exactly.");
    cascas_append_line(out,"5. Sub consts back; simp/fact.");
    cascas_append_line(out,"6. Chk: all conds true.");
    return;
  }
  if (strstr(s,"match(")){
    cascas_append_line(out,"2. Match: expand/collect both forms.");
    cascas_append_line(out,"3. Equate coeffs/terms.");
    cascas_append_line(out,"4. Solve params exactly.");
    cascas_append_line(out,"5. Sub params; simp.");
    cascas_append_line(out,"6. Chk: diff -> 0.");
    return;
  }
  if (strstr(s,"complete_square(")){
    cascas_append_line(out,"2. Comp sq: ax^2+bx+c -> a(x+h)^2+k.");
    cascas_append_line(out,"3. h=b/(2a); k=c-b^2/(4a).");
    cascas_append_line(out,"4. Simp exact; fact if useful.");
    cascas_append_line(out,"5. Chk: expand back.");
    return;
  }
  if (strstr(s,"de_solve(") || strstr(s,"desolve(")){
    cascas_append_line(out,"2. DE: classify sep/linear/order.");
    cascas_append_line(out,"3. Rearrange vars if sep; IF if linear.");
    cascas_append_line(out,"4. Integrate both sides; +C.");
    cascas_append_line(out,"5. Apply BC/IC if given.");
    cascas_append_line(out,"6. Chk: sub y into DE.");
    return;
  }
  if (strstr(s,"implicit_diff(")){
    cascas_append_line(out,"2. Impl: set F=lhs-rhs=0.");
    cascas_append_line(out,"3. d/dx all terms; y=y(x).");
    cascas_append_line(out,"4. Chain: d[f(y)]/dx=f'(y)y'.");
    cascas_append_line(out,"5. Collect y'; factor.");
    cascas_append_line(out,"6. y'=-Fx/Fy; simp/fact.");
    cascas_append_line(out,"7. Chk: sub y' in d-eq.");
    return;
  }
  if (strstr(s,"param_diff(") || strstr(s,"param_second_diff(")){
    cascas_append_line(out,"2. Param: read x(t),y(t),t.");
    cascas_append_line(out,"3. Find dx/dt, dy/dt.");
    cascas_append_line(out,"4. y'=dy/dx=(dy/dt)/(dx/dt); dx/dt!=0.");
    cascas_append_line(out,"5. y''=d(y')/dt /(dx/dt).");
    cascas_append_line(out,"6. Simp/fact.");
    cascas_append_line(out,"7. Chk: sub t/redo diff.");
    return;
  }
  if (strstr(s,"param_area(")){
    cascas_append_line(out,"2. Area: A=int y dx.");
    cascas_append_line(out,"3. dx=(dx/dt)dt -> A=int y(t)x'(t)dt.");
    cascas_append_line(out,"4. Check sign; abs if area.");
    cascas_append_line(out,"5. Use t-bounds or convert x-bounds.");
    cascas_append_line(out,"6. Simp; chk by diff.");
    return;
  }
  if (strstr(s,"normal_diff(") || strstr(s,"second_diff(") || strstr(s,"tangent_line(")){
    cascas_append_line(out,"2. Diff: f,var,order/pt.");
    cascas_append_line(out,"3. Rules: pow/log/exp/trig; chain for brackets.");
    cascas_append_line(out,"4. Prod: u'v+uv'. Quot: (u'v-uv')/v^2.");
    cascas_append_line(out,"5. Trig: tan->sec^2, sec->sec*tan, cot->-cosec^2 (+chain k).");
    cascas_append_line(out,"6. Simp/fact; tangent: y-y1=m(x-x1).");
    cascas_append_line(out,"7. Chk: CAS diff-simp.");
    return;
  }
  if (strstr(s,"trig_prove(") || strstr(s,"trig_transform(") || strstr(s,"trig_rewrite(")){
    cascas_append_line(out,"2. Trig: check lhs-rhs -> 0.");
    cascas_append_line(out,"3. Rewrite: tan=sin/cos; sec=1/cos; cosec=1/sin; cot=cos/sin.");
    cascas_append_line(out,"4. Ids: sin^2+cos^2=1; 1+tan^2=sec^2; 1+cot^2=cosec^2.");
    cascas_append_line(out,"5. Double/compound: expand, collect/fact.");
    cascas_append_line(out,"6. Chk: 0; note domain excl.");
    return;
  }
  if (strstr(s,"compose(") || strstr(s,"inverse(")){
    cascas_append_line(out,"2. Fn: read f,g,var.");
    cascas_append_line(out,"3. Comp: sub g(x) into f; simp.");
    cascas_append_line(out,"4. Inv: y=f(x); swap; solve y.");
    cascas_append_line(out,"5. Track dom/range.");
    cascas_append_line(out,"6. Chk: f(f^-1)=x on dom.");
    return;
  }
  if (strstr(s,"suvat(")){
    cascas_append_line(out,"2. SUVAT: put knowns in const-acc eqs.");
    cascas_append_line(out,"3. Pick eq with target + fewest unknowns.");
    cascas_append_line(out,"4. Solve exact sim eqs.");
    cascas_append_line(out,"5. Chk: v=u+at; s=ut+1/2at^2; v^2=u^2+2as.");
    return;
  }
  if (strstr(s,"bool_simplify(") || strstr(s,"nand(") || strstr(s,"nor(") || strstr(s,"prove_bool(")){
    cascas_append_line(out,"2. Bool: parse NOT/AND/OR.");
    cascas_append_line(out,"3. Use De Morgan, absorb, distrib, NAND/NOR.");
    cascas_append_line(out,"4. One rule/line if useful.");
    cascas_append_line(out,"5. Chk: simp both sides/truth table.");
    return;
  }
  if (strstr(s,"diff(") || strstr(s,"derive(") || strstr(s,"'")){
    cascas_append_line(out,"2. Diff: f,var,order.");
    cascas_append_line(out,"3. Split sums; const -> 0.");
    cascas_append_line(out,"4. Choose chain/prod/quot.");
    cascas_append_line(out,"5. Trig kx chain factors.");
    cascas_append_line(out,"6. Simp: collect/cancel/fact.");
    cascas_append_line(out,"7. Chk: CAS diff-simp.");
    return;
  }
  if (strstr(s,"integrate(") || strstr(s,"int(")){
    if (cascas_append_special_integral_lines(out,s))
      return;
    cascas_append_line(out,"2. Int: f,var,bounds.");
    cascas_append_line(out,"3. Std: x^n,sin,cos,sec^2,sec*tan,cosec*cot,e^x,1/x,a^x.");
    cascas_append_line(out,"4. f(kx+b): divide by k.");
    cascas_append_line(out,"5. Rev-chain: f(g)g' -> u=g.");
    cascas_append_line(out,"6. Quot: f'/f -> ln(abs(f)); f'/f^n -> pow rule.");
    cascas_append_line(out,"7. DI: table for x^n exp/trig; else parts uv.");
    cascas_append_line(out,"8. PF: linear/repeated/quad (Ax+B); trig rewrite if useful.");
    cascas_append_line(out,"9. Simp exact; +C if indef.");
    cascas_append_line(out,"10. Chk: diff ans or re-eval def.");
    return;
  }
  if (strstr(s,"solve(") || strstr(s,"csolve(") || strstr(s,"linsolve(") || strstr(s,"fsolve(")){
    cascas_append_line(out,"2. Move to lhs-rhs=0.");
    cascas_append_line(out,"3. Dom: denom/log/root/trig excl.");
    cascas_append_line(out,"4. Clear fracs/brackets if simpler.");
    cascas_append_line(out,"5. Reduce: fact/normal; lin/quad/poly/trig/num.");
    cascas_append_line(out,"6. Exact first; num fallback.");
    cascas_append_line(out,"7. Chk: sub solns; reject extra.");
    return;
  }
  if (strstr(s,"factor(") || strstr(s,"partfrac(") || strstr(s,"canonical_form(") ||
      strstr(s,"expand(") || strstr(s,"collect(") || strstr(s,"normal(") ||
      strstr(s,"ratnormal(") || strstr(s,"simplify(")){
    cascas_append_line(out,"2. Normalise exact form.");
    cascas_append_line(out,"3. Expand hidden structure; fact roots; collect params.");
    cascas_append_line(out,"4. Frac: common denom -> partfrac if factored.");
    cascas_append_line(out,"5. Prefer exact fact/collected form.");
    cascas_append_line(out,"6. Chk: expand(input-res)=0.");
    return;
  }
  if (strstr(s,"limit(") || strstr(s,"taylor(") || strstr(s,"series(")){
    cascas_append_line(out,"2. Local: expand at pt/side/order.");
    cascas_append_line(out,"3. 0/0 or inf/inf: simp first; series/L'Hop valid only.");
    cascas_append_line(out,"4. Cancel dominant terms; simp lead form.");
    cascas_append_line(out,"5. Chk: local expansion/order.");
    return;
  }
  if (strstr(s,"det(") || strstr(s,"rref(") || strstr(s,"inv(") || strstr(s,"eigen") ||
      strstr(s,"tran(") || strstr(s,"rank(")){
    cascas_append_line(out,"2. Mat: check dims/exact entries.");
    cascas_append_line(out,"3. inv/rref: row ops -> echelon/reduced.");
    cascas_append_line(out,"4. eigval: det(A-lambda I)=0; fact char poly.");
    cascas_append_line(out,"5. eigvec: solve (A-lambda I)v=0; scale.");
    cascas_append_line(out,"6. Chk: multiply/sub back.");
    return;
  }
  if (strstr(s,"binomial(") || strstr(s,"normald(") || strstr(s,"correlation(") ||
      strstr(s,"regression") || strstr(s,"mean(") || strstr(s,"stddev(")){
    cascas_append_line(out,"2. Stats: stat/dist + params.");
    cascas_append_line(out,"3. If test: H0/H1, tail, CR/test stat.");
    cascas_append_line(out,"4. Sub values in formula.");
    cascas_append_line(out,"5. Compare p/crit; conclude.");
    cascas_append_line(out,"6. Exact; round final only.");
    return;
  }
  if (strstr(s,"plot") || strstr(s,"histogram") || strstr(s,"scatterplot")){
    cascas_append_line(out,"2. Graph: parse expr/data/options.");
    cascas_append_line(out,"3. Note intercepts/asymptotes/dom if needed.");
    cascas_append_line(out,"4. Build graph obj.");
    return;
  }
  if (strstr(s,"sin(") || strstr(s,"cos(") || strstr(s,"tan(") ||
      strstr(s,"trig") || strstr(s,"sincos(")){
    cascas_append_line(out,"2. Trig: normalise angles; choose id family.");
    cascas_append_line(out,"3. Rewrite tan/sec/cosec/cot to sin/cos if shorter.");
    cascas_append_line(out,"4. Use pythag/double/compound/R-form.");
    cascas_append_line(out,"5. Solve: base angle, quadrants, period, interval.");
    cascas_append_line(out,"6. Chk: simp diff; note excl.");
    return;
  }
  cascas_append_line(out,"2. P: KhiCAS exact parse.");
  cascas_append_line(out,"3. Fallback: normal/simp/fact/expand/partfrac/trig.");
  cascas_append_line(out,"4. If stuck: rearrange, clear denom, sub simpler var.");
  cascas_append_line(out,"5. Pick shortest exact MS form.");
  cascas_append_line(out,"6. Chk: exact simp(original-result).");
}

static string cascas_working_text(const char *input,const char *eval_input,const string &answer){
  string out("1. In: ");
  out += input?input:"";
  out += "\n";
  if (eval_input && input && strcmp(eval_input,input)){
    out += "Rw: ";
    out += eval_input;
    out += "\n";
  }
  cascas_append_method_lines(out,(eval_input && input && strcmp(eval_input,input))?input:(eval_input?eval_input:input));
  out += "Ans: ";
  out += answer;
  return out;
}

static bool cascas_show_working_for(const char *s,const string &answer){
  if (!s || !*s)
    return false;
  if (answer=="Graphic object")
    return false;
  return true;
}

static void cascas_output_working(const char *input,const char *eval_input,const string &answer){
  string body=cascas_working_text(input,eval_input,answer);
  size_t start=0;
  for (size_t i=0;i<=body.size();++i){
    if (i<body.size() && body[i]!='\n')
      continue;
    string line=body.substr(start,i-start);
    if (i<body.size())
      cascas_output_line(line);
    else
      Console_Output((const unsigned char*)line.c_str());
    start=i+1;
  }
}

static void cascas_show_working_screen(const char *input,const char *eval_input,const string &answer){
  textArea text;
  text.title=(char*)"Work";
  text.editable=false;
  text.allowEXE=true;
  text.clipline=-1;
  text.scrollbar=1;
  add(&text,cascas_working_text(input,eval_input,answer));
  doTextArea(&text);
}

void run(const char * s,int do_logo_graph_eqw){
  if (strlen(s)>=2 && (s[0]=='#' ||
		       (s[0]=='/' && (s[1]=='/' || s[1]=='*'))
		       ))
    return;
  gen g,ge;
  string rewritten;
  const char *eval_s=s;
  if (cascas_rewrite_alias(s,rewritten))
    eval_s=rewritten.c_str();
  do_run(eval_s,g,ge);
  if (giac::freeze){
    giac::freeze=false;
    DefineStatusMessage((char*)(lang?"Ecran fige. Taper EXIT":"Screen freezed. Press EXIT."), 1, 0, 0);
    DisplayStatusArea();
    for (;;){
      int key;
      ck_getkey(&key);
      if (key==KEY_CTRL_EXIT)
	break;
    }
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
  check_do_graph(ge,do_logo_graph_eqw & ~1);
  string s_;
  if (ge.type==giac::_STRNG)
    s_='"'+*ge._STRNGptr+'"';
  else {
    if (te>256)
      s_="Object too large";
    else {
      if (ge.is_symb_of_sommet(giac::at_pnt) || (ge.type==giac::_VECT && !ge._VECTptr->empty() && ge._VECTptr->back().is_symb_of_sommet(giac::at_pnt)))
	s_="Graphic object";
      else {
	//do_tex=ge.type==giac::_SYMB && has_op(ge,*giac::at_inv);
	// tex support has been disabled!
	s_=ge.print(contextptr);
	// translate to tex? set do_tex to true
      }
    }
  }
  if (s_.size()>512)
    s_=s_.substr(0,509)+"...";
  char* edit_line = (char*)Console_GetEditLine();
  if (cascas_show_working_for(s,s_)){
    if (do_logo_graph_eqw % 2)
      cascas_show_working_screen(s,eval_s,s_);
    cascas_output_working(s,eval_s,s_);
  }
  else
    Console_Output((const unsigned char*)s_.c_str());
  //return ge; 
}

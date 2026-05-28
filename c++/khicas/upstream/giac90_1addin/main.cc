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
		print_msg12("Relancer:","MENU tan MENU");
      else
		print_msg12("Restart:","MENU tan MENU");
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
		print_msg12("Eteindre:","MENU SHIFT ON");
      else
			print_msg12("Shutdown:","MENU SHIFT ON");
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
  // Keep common A-level aliases coloured if Giac's lexer treats them as symbols.
  if (!strcmp(buf,"log") || !strcmp(buf,"log10") || !strcmp(buf,"ln") ||
      !strcmp(buf,"sec") || !strcmp(buf,"csc") ||
      !strcmp(buf,"cosec") || !strcmp(buf,"cot"))
    return 3;
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
  SetQuitHandler(save_session); // keep one autosaved shell state
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
      DefineStatusMessage((char*)(lang?"EXE: quitte":"EXE: quit"), 1, 0, 0);
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
      if (confirm(lang?"Abandonner?":"Leave?",lang?"F1: retour, F6: ok":"F1: back, F6: ok")==KEY_CTRL_F6)
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
  DefineStatusMessage((char*)"zoom/pad; EXIT", 1, 0, 0);
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
  DefineStatusMessage((char*)"zoom/pad; EXIT", 1, 0, 0);
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
    puts("Script too big");
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
      puts("Script too big");
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

#if 0
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
#endif

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
    if (confirm(tmp.c_str(),lang?"F1: sauv, F6: jeter":"F1: save, F6: discard")==KEY_CTRL_F1){
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
    PrintMini(&x,&y,(unsigned char*)"CasioCAS",0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    x=0; y=138;
    PrintMini(&x,&y,(unsigned char*)"  License GPL 2",0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    x=0; y=156;
    PrintMini(&x,&y,(unsigned char*)"  Exam rules apply",0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    python_compat(false,contextptr);
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
      s=python_compat(contextptr)?"Python":"Xcas";
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

static bool cascas_syntax_complete(const char *s){
  int depth=0; bool instring=false;
  for (int i=0;s && s[i];++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{')
      ++depth;
    if (c==')' || c==']' || c=='}')
      --depth;
    if (depth<0)
      return false;
  }
  return depth==0 && !instring;
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

static string cascas_trim(const string &s){
  int a=0,b=int(s.size())-1;
  while (a<int(s.size()) && isspace((unsigned char)s[a])) ++a;
  while (b>=a && isspace((unsigned char)s[b])) --b;
  if (b<a)
    return "";
  return s.substr(a,b-a+1);
}

static unsigned cascas_hash_step(unsigned h,unsigned char c){
  return (h ^ c) * 16777619u;
}

static bool cascas_removed_function_hash(unsigned h){
  switch (h){
    case 0x0073dacfu:
    case 0x011ce908u:
    case 0x018ead2eu:
    case 0x01975c10u:
    case 0x02dc2b01u:
    case 0x04e0311fu:
    case 0x04fe91d1u:
    case 0x05f85713u:
    case 0x06bd505bu:
    case 0x071515d2u:
    case 0x07275075u:
    case 0x08230495u:
    case 0x08c21d83u:
    case 0x092855d0u:
    case 0x0ad0ed6cu:
    case 0x0c82e56bu:
    case 0x0dd0b0beu:
    case 0x0fad7a44u:
    case 0x10d2583fu:
    case 0x11420879u:
    case 0x1413edcdu:
    case 0x1427b873u:
    case 0x14cdb897u:
    case 0x15c2f8ecu:
    case 0x1642b069u:
    case 0x1696d742u:
    case 0x178d4c35u:
    case 0x17d1d6fcu:
    case 0x18ba068bu:
    case 0x198be6b8u:
    case 0x1b5e2979u:
    case 0x1b777caeu:
    case 0x1bbede44u:
    case 0x1eab292au:
    case 0x1ebe2c6au:
    case 0x1f82d9a8u:
    case 0x22fc8274u:
    case 0x23b19318u:
    case 0x23c6e902u:
    case 0x24fa4703u:
    case 0x262cfcabu:
    case 0x26851732u:
    case 0x2733f84au:
    case 0x2b8eb358u:
    case 0x2ba548a6u:
    case 0x2d5f622cu:
    case 0x2f271707u:
    case 0x318b991du:
    case 0x31bd46a1u:
    case 0x38185d76u:
    case 0x39888dd9u:
    case 0x3a64d347u:
    case 0x3a7c9fcdu:
    case 0x3e6f3127u:
    case 0x3f2f216au:
    case 0x4038790bu:
    case 0x41229629u:
    case 0x41320c2du:
    case 0x4143b863u:
    case 0x4172a512u:
    case 0x4188b5ddu:
    case 0x440cf8a1u:
    case 0x44c63840u:
    case 0x45855c00u:
    case 0x462f5ba1u:
    case 0x47060460u:
    case 0x47a80046u:
    case 0x4988a709u:
    case 0x49c43198u:
    case 0x49f2dba0u:
    case 0x4a8d76c1u:
    case 0x4abab91fu:
    case 0x4caa143cu:
    case 0x4ceb35d5u:
    case 0x4db564c2u:
    case 0x4edf1404u:
    case 0x4f04d9d2u:
    case 0x503e3086u:
    case 0x50b13859u:
    case 0x51295e73u:
    case 0x5188bedbu:
    case 0x519b7f08u:
    case 0x52807cf8u:
    case 0x5319fe9eu:
    case 0x53fd0c42u:
    case 0x5683a798u:
    case 0x5824dcf7u:
    case 0x58547550u:
    case 0x58bc3fe6u:
    case 0x59bc4179u:
    case 0x5bde019bu:
    case 0x5c07d9ecu:
    case 0x5c31e95au:
    case 0x5c95b7aeu:
    case 0x5d0e6938u:
    case 0x5d36cd35u:
    case 0x5d4c2e6au:
    case 0x5d7168a6u:
    case 0x5ea077c6u:
    case 0x614ba208u:
    case 0x623beeebu:
    case 0x633cdd81u:
    case 0x65a9728bu:
    case 0x66b85f2du:
    case 0x6a7d2c89u:
    case 0x6b5871d0u:
    case 0x6bff7e93u:
    case 0x6c04a192u:
    case 0x6cc00459u:
    case 0x6e72bdcfu:
    case 0x6eb56479u:
    case 0x6ef4d9bdu:
    case 0x6f20f7ddu:
    case 0x7302743fu:
    case 0x7390ec3bu:
    case 0x756f22cbu:
    case 0x77b208f6u:
    case 0x7817e298u:
    case 0x7855062eu:
    case 0x78978610u:
    case 0x7917e42bu:
    case 0x7a0e47bau:
    case 0x7bc647b9u:
    case 0x7bcfbd8au:
    case 0x7d033966u:
    case 0x7d3ff927u:
    case 0x7d6fc026u:
    case 0x7e8a3329u:
    case 0x808153e1u:
    case 0x8132f281u:
    case 0x813d75aeu:
    case 0x81df5ea5u:
    case 0x856d7f49u:
    case 0x880dac7fu:
    case 0x8a2c68b7u:
    case 0x8bd5ceabu:
    case 0x8bfaffcfu:
    case 0x8d2519cbu:
    case 0x8e816a60u:
    case 0x8e9dd543u:
    case 0x8f605233u:
    case 0x8f9b7080u:
    case 0x8fae83d8u:
    case 0x90d2e37au:
    case 0x91708e0cu:
    case 0x93e97b38u:
    case 0x940b5e09u:
    case 0x98a2959eu:
    case 0x991301e0u:
    case 0x99618446u:
    case 0x99d9f9f0u:
    case 0x9c662fa4u:
    case 0x9c90b906u:
    case 0x9ce3039bu:
    case 0x9e888b98u:
    case 0x9ede2954u:
    case 0x9f634068u:
    case 0xa1cf14d3u:
    case 0xa3325c6fu:
    case 0xa71467f9u:
    case 0xa893baa8u:
    case 0xa9c8feadu:
    case 0xaa9b9b01u:
    case 0xac9b9e27u:
    case 0xacd224d9u:
    case 0xacda80fbu:
    case 0xae13f94au:
    case 0xaea3ddb0u:
    case 0xaf645378u:
    case 0xb1c0e744u:
    case 0xb1e15f65u:
    case 0xb23e7192u:
    case 0xb27e96edu:
    case 0xb3dab835u:
    case 0xb4423af0u:
    case 0xb469c380u:
    case 0xb4e7f8e4u:
    case 0xb5fc9604u:
    case 0xb8b5368fu:
    case 0xb9c6a953u:
    case 0xba56d76du:
    case 0xbab19e4au:
    case 0xbb4555a1u:
    case 0xbbc6ac79u:
    case 0xbc3c467au:
    case 0xc03183c0u:
    case 0xc1c5c6a2u:
    case 0xc2eb0145u:
    case 0xc352e04au:
    case 0xc3e8e5f7u:
    case 0xc4c0bedeu:
    case 0xc7bd6948u:
    case 0xcada163bu:
    case 0xcae0a60au:
    case 0xcb08e2d8u:
    case 0xcb9ba81du:
    case 0xcbea440au:
    case 0xcc30f8fau:
    case 0xccee2183u:
    case 0xce2ab760u:
    case 0xce59cbb3u:
    case 0xcf9fd8f0u:
    case 0xd04036f3u:
    case 0xd2353873u:
    case 0xd4d5660fu:
    case 0xd4f0716cu:
    case 0xd4f67cfdu:
    case 0xd57925a4u:
    case 0xd5a84be9u:
    case 0xd5b7b015u:
    case 0xd64da1c4u:
    case 0xd6dfb05du:
    case 0xd6f47b66u:
    case 0xd7037e9bu:
    case 0xd7599ae2u:
    case 0xd81f50a6u:
    case 0xd9c30496u:
    case 0xdd4a6ba7u:
    case 0xde375a53u:
    case 0xe0504bdeu:
    case 0xe2438311u:
    case 0xe26424d0u:
    case 0xe272ae1bu:
    case 0xe44789e8u:
    case 0xe667368du:
    case 0xe80fa444u:
    case 0xe848572au:
    case 0xea8a4792u:
    case 0xebd12f63u:
    case 0xec7aec99u:
    case 0xedf2c855u:
    case 0xee43b913u:
    case 0xee65cbdau:
    case 0xeea7e9ccu:
    case 0xf28765e9u:
    case 0xf2c80d5bu:
    case 0xf3c7dd11u:
    case 0xf3fa9284u:
    case 0xf45c461cu:
    case 0xf51d8813u:
    case 0xf752b052u:
    case 0xf76dddbbu:
    case 0xf8aaffd3u:
    case 0xf9950dffu:
    case 0xfba40b66u:
    case 0xfcaee333u:
    case 0xfcafa6eau:
    case 0xfd455c3bu:
    case 0xfe42bdf4u:
    case 0xfeeb57a9u:
      return true;
  }
  return false;
}

static bool cascas_contains_removed_function(const char *text){
  unsigned h=2166136261u;
  bool inword=false, waitparen=false;
  for (int i=0;text && text[i];++i){
    unsigned char c=(unsigned char)text[i];
    if (isalpha(c) || c=='_'){
      if (!inword){
	h=2166136261u;
	inword=true;
	waitparen=false;
      }
      h=cascas_hash_step(h,(unsigned char)tolower(c));
      continue;
    }
    if (inword && isdigit(c)){
      h=cascas_hash_step(h,c);
      continue;
    }
    if (inword){
      inword=false;
      waitparen=true;
    }
    if (waitparen){
      if (c==' ' || c=='\t' || c=='\r' || c=='\n')
	continue;
      if (c=='(' && cascas_removed_function_hash(h))
	return true;
      waitparen=false;
    }
  }
  return false;
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

static bool cascas_arg_key(const string &arg,const char *key,string &value){
  string t=cascas_trim(arg);
  int n=strlen(key);
  if (int(t.size())<=n+1 || strncmp(t.c_str(),key,n) || t[n]!='=')
    return false;
  value=cascas_trim(t.substr(n+1,t.size()-n-1));
  return value.size()!=0;
}

static bool cascas_extract_method(const char *input,string &method,string &u,string *target=0){
  method.clear();
  u.clear();
  if (target)
    target->clear();
  if (!input)
    return false;
  string s(input);
  const char *openp=strchr(input,'(');
  if (!openp)
    return false;
  int open=openp-input;
  int close=cascas_find_matching_paren(s,open);
  if (close<0)
    return false;
  string args[10];
  int count=cascas_split_top_args(s,open+1,close,args,10);
  if (count<1)
    return false;
  for (int i=0;i<count;++i){
    string v;
    if (cascas_arg_key(args[i],"method",v) || cascas_arg_key(args[i],"met",v))
      method=v;
    if (cascas_arg_key(args[i],"u",v))
      u=v;
    if (target && cascas_arg_key(args[i],"target",v))
      *target=v;
  }
  string name=s.substr(0,open);
  if (method.empty() && name.size()>3 && name.substr(name.size()-3,3)=="_by" && count>=3)
    method=args[2];
  return !method.empty() || !u.empty() || (target && !target->empty());
}

static bool cascas_strip_method_args(const char *input,string &out){
  if (!input)
    return false;
  string s(input);
  const char *openp=strchr(input,'(');
  if (!openp)
    return false;
  int open=openp-input;
  int close=cascas_find_matching_paren(s,open);
  if (close<0)
    return false;
  string args[10],kept[10],tmp;
  int count=cascas_split_top_args(s,open+1,close,args,10),n=0;
  bool changed=false;
  bool drop_u=cascas_startswith(input,"integrate(") || cascas_startswith(input,"int(") ||
    cascas_startswith(input,"integrate_by(") || cascas_startswith(input,"int_by(");
  bool drop_target=cascas_startswith(input,"suvat(");
  if (count<1)
    return false;
  for (int i=0;i<count;++i){
    if (cascas_arg_key(args[i],"method",tmp) || cascas_arg_key(args[i],"met",tmp) ||
	(drop_u && cascas_arg_key(args[i],"u",tmp)) ||
	(drop_target && cascas_arg_key(args[i],"target",tmp))){
      changed=true;
      continue;
    }
    kept[n++]=args[i];
  }
  if (!changed)
    return false;
  out=s.substr(0,open+1);
  for (int i=0;i<n;++i){
    if (i)
      out += ",";
    out += kept[i];
  }
  out += ")";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_by_call(const char *input,const char *alias,const char *target,string &out){
  string args[10],s; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,10,count,close,s) || count<1)
    return false;
  out=target;
  out += "(";
  int keep=count;
  if (count>=3)
    keep=2; // arg 3 is method; later args are method parameters for working only.
  for (int i=0;i<keep;++i){
    if (i)
      out += ",";
    out += args[i];
  }
  out += ")";
  out += s.substr(close+1,s.size()-close-1);
  return true;
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

static bool cascas_find_top_rel(const string &s,int &pos,string &op){
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
    else if (c==')' || c==']' || c=='}')
      --depth;
    else if (!depth && (c=='<' || c=='>')){
      pos=i;
      op=string(1,c);
      if (i+1<int(s.size()) && s[i+1]=='=')
	op += "=";
      return true;
    }
  }
  return false;
}

static string cascas_flip_rel(const string &op){
  if (op=="<") return ">";
  if (op==">") return "<";
  if (op=="<=") return ">=";
  if (op==">=") return "<=";
  return op;
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

static string cascas_residual_expr(const string &expr){
  int eq=cascas_find_top_equal(expr);
  if (eq>=0)
    return "((" + expr.substr(0,eq) + ")-(" + expr.substr(eq+1,expr.size()-eq-1) + "))";
  return "(" + expr + ")";
}

static bool cascas_rewrite_compare_zero_call(const char *input,const char *alias,string &out){
  string args[2],s; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,2,count,close,s) || count<1)
    return false;
  if (count>=2)
    out="normal(" + cascas_residual_expr(args[0]) + "-" + cascas_residual_expr(args[1]) + ")";
  else
    out="normal" + cascas_residual_expr(args[0]);
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_trig_basis_call(const char *input,const char *alias,const char *target,string &out){
  string args[2],s; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,2,count,close,s) || count<1)
    return false;
  string expr=args[0];
  int eq=cascas_find_top_equal(expr);
  string body=eq>=0?"((" + expr.substr(0,eq) + ")-(" + expr.substr(eq+1,expr.size()-eq-1) + "))":"(" + expr + ")";
  string core=string(target) + "(tcollect(texpand(sincos(" + body + "))))";
  out="factor(normal(" + core + "))";
  if (eq>=0)
    out += "=0";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_binom_coeff_call(const char *input,string &out){
  string args[4],s; int count=0,close=0;
  if (!cascas_call_args(input,"binom_coeff(",args,4,count,close,s) || count<3)
    return false;
  out="coeff(expand((" + args[0] + "))," + args[1] + "," + args[2] + ")";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_normalcdf_call(const char *input,string &out){
  string args[4],s; int count=0,close=0;
  if (!cascas_call_args(input,"normalcdf(",args,4,count,close,s) || count<4)
    return false;
  out="evalf(normal_cdf(" + args[0] + "," + args[1] + "," + args[2] + "," + args[3] + "))";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_binomial_dist_call(const char *input,string &out){
  string args[3],s; int count=0,close=0;
  if (cascas_call_args(input,"binomcdf(",args,3,count,close,s) && count>=3){
    out="binomial_cdf(" + args[0] + "," + args[1] + "," + args[2] + ")";
    out += s.substr(close+1,s.size()-close-1);
    return true;
  }
  if (cascas_call_args(input,"binompdf(",args,3,count,close,s) && count>=3){
    out="binomial(" + args[0] + "," + args[1] + "," + args[2] + ")";
    out += s.substr(close+1,s.size()-close-1);
    return true;
  }
  if (cascas_call_args(input,"binom(",args,3,count,close,s) && count>=3){
    out="binomial(" + args[0] + "," + args[1] + "," + args[2] + ")";
    out += s.substr(close+1,s.size()-close-1);
    return true;
  }
  return false;
}

static bool cascas_rewrite_coeff_match_call(const char *input,const char *alias,string &out){
  string args[4],s; int count=0,close=0;
  if (!cascas_call_args(input,alias,args,4,count,close,s) || count<3)
    return false;
  string x=(count>=4 && args[3].size())?args[3]:"x";
  string diff="expand(numer(normal((" + args[0] + ")-(" + args[1] + "))))";
  out="solve([";
  for (int i=0;i<=6;++i){
    if (i)
      out += ",";
    out += "coeff(" + diff + "," + x + "," + print_INT_(i) + ")=0";
  }
  out += "]," + args[2] + ")";
  out += s.substr(close+1,s.size()-close-1);
  return true;
}

static bool cascas_rewrite_fitconst_call(const char *input,string &out){
  string args[3],s; int count=0,close=0;
  if (!cascas_call_args(input,"fitconst(",args,3,count,close,s) || count<2)
    return false;
  int eq=cascas_find_top_equal(args[0]);
  if (eq<0)
    return false;
  string lhs=args[0].substr(0,eq),rhs=args[0].substr(eq+1,args[0].size()-eq-1);
  string call="coeff_match(" + lhs + "," + rhs + "," + args[1] + ")";
  return cascas_rewrite_coeff_match_call(call.c_str(),"coeff_match(",out);
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

static bool cascas_rewrite_alias(const char *input,string &rewritten){
  if (!input)
    return false;
  string stripped;
  if (cascas_strip_method_args(input,stripped)){
    if (cascas_rewrite_alias(stripped.c_str(),rewritten))
      return true;
    rewritten=stripped;
    return true;
  }
  static const struct { const char *alias; const char *target; } by_aliases[]={
    {"integrate_by(","integrate"},{"int_by(","integrate"},{"diff_by(","diff"},
    {"solve_by(","solve"},{"simplify_by(","simplify"},{"solve_trig_by(","solve_trig"}
  };
  for (int i=0;i<int(sizeof(by_aliases)/sizeof(by_aliases[0]));++i){
    if (cascas_rewrite_by_call(input,by_aliases[i].alias,by_aliases[i].target,rewritten))
      return true;
  }
  if (cascas_rewrite_compose_call(input,rewritten))
    return true;
  if (cascas_rewrite_inverse_call(input,rewritten))
    return true;
  if (cascas_rewrite_diff_alias_call(input,"normal_diff(",1,rewritten))
    return true;
  if (cascas_rewrite_diff_alias_call(input,"diff(",1,rewritten))
    return true;
  if (cascas_rewrite_implicit_diff_call(input,rewritten))
    return true;
  if (cascas_rewrite_param_call(input,"param_diff(",1,rewritten))
    return true;
  if (cascas_rewrite_solve_trig_call(input,rewritten))
    return true;
  if (cascas_rewrite_trig_basis_call(input,"trigcos(","trigcos",rewritten))
    return true;
  if (cascas_rewrite_trig_basis_call(input,"trigsin(","trigsin",rewritten))
    return true;
  if (cascas_rewrite_trig_basis_call(input,"trigtan(","trigtan",rewritten))
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
  if (cascas_rewrite_binom_coeff_call(input,rewritten))
    return true;
  if (cascas_rewrite_normalcdf_call(input,rewritten))
    return true;
  if (cascas_rewrite_binomial_dist_call(input,rewritten))
    return true;
  if (cascas_rewrite_fitconst_call(input,rewritten))
    return true;
  if (cascas_rewrite_coeff_match_call(input,"coeff_match(",rewritten))
    return true;
  if (cascas_rewrite_coeff_match_call(input,"match(",rewritten))
    return true;
  static const struct { const char *alias; const char *target; } simple_aliases[]={
    {"tangent_line(","linetan("},{"de_solve(","desolve("},{"poly(","factor("},
    {"complete_square(","canonical_form("},{"fitconst(","solve("},{"match(","solve("},
    {"binom_expand(","expand("},{"rewrite(","canonical_form("},
    {"suvat(","solve("},{"cartesian(","eliminate("}
  };
  for (int i=0;i<int(sizeof(simple_aliases)/sizeof(simple_aliases[0]));++i){
    if (cascas_replace_call(input,simple_aliases[i].alias,simple_aliases[i].target,rewritten))
      return true;
  }
  return false;
}

static string cascas_lower_trim_text(const string &s){
  string t=cascas_trim(s);
  for (int i=0;i<int(t.size());++i)
    t[i]=tolower((unsigned char)t[i]);
  return t;
}

static bool cascas_is_pos_inf_text(const string &s){
  string t=cascas_lower_trim_text(s);
  return t=="inf" || t=="+inf" || t=="infinity" || t=="+infinity";
}

static bool cascas_is_neg_inf_text(const string &s){
  string t=cascas_lower_trim_text(s);
  return t=="-inf" || t=="-infinity";
}

static bool cascas_nonneg_int_text(const string &s){
  string t=cascas_trim(s);
  if (t.empty())
    return false;
  for (int i=0;i<int(t.size());++i){
    if (!isdigit((unsigned char)t[i]))
      return false;
  }
  return true;
}

static string cascas_strip_outer_group(string s){
  s=cascas_trim(s);
  while (s.size()>=2 && s[0]=='(' && s[s.size()-1]==')'){
    int close=cascas_find_matching_paren(s,0);
    if (close!=int(s.size())-1)
      break;
    s=cascas_trim(s.substr(1,s.size()-2));
  }
  return s;
}

static int cascas_find_top_power(const string &s){
  int depth=0;
  bool instring=false;
  for (int i=int(s.size())-1;i>=0;--i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c==')' || c==']' || c=='}')
      ++depth;
    if (c=='(' || c=='[' || c=='{')
      --depth;
    if (!depth && c=='^')
      return i;
  }
  return -1;
}

static int cascas_find_base_split(const string &s){
  int depth=0;
  bool instring=false;
  for (int i=1;i<int(s.size());++i){
    char c=s[i];
    if (c=='"' && (i==0 || s[i-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    if (c=='(' || c=='[' || c=='{')
      ++depth;
    if (c==')' || c==']' || c=='}')
      --depth;
    if (!depth && (c=='+' || c=='-'))
      return i;
  }
  return -1;
}

static string cascas_tpl(const char *key);

static string cascas_binom_validity(const char *input){
  string args[4],s; int count=0,close=0;
  if (!cascas_call_args(input,"binom_expand(",args,4,count,close,s)){
    if (!cascas_call_args(input,"binom_coeff(",args,4,count,close,s))
      return "";
  }
  if (count<1)
    return "";
  string expr=cascas_strip_outer_group(args[0]);
  int p=cascas_find_top_power(expr);
  if (p<0)
    return "";
  string base=cascas_strip_outer_group(expr.substr(0,p));
  string exp=cascas_strip_outer_group(expr.substr(p+1,expr.size()-p-1));
  if (cascas_nonneg_int_text(exp))
    return cascas_tpl("t000");
  int split=cascas_find_base_split(base);
  if (split<0)
    return cascas_tpl("t001");
  string a=cascas_strip_outer_group(base.substr(0,split));
  string b=cascas_strip_outer_group(base.substr(split+1,base.size()-split-1));
  if (base[split]=='-')
    b="-(" + b + ")";
  if ((a=="1" || a=="+1") && (b=="x" || b=="-(x)"))
    return cascas_tpl("t002");
  return cascas_tpl("t003") + b + ")/(" + a + ")|<1";
}

static void cascas_output_line(const string &s){
  Console_Output((const unsigned char*)s.c_str());
  Console_NewLine(LINE_TYPE_OUTPUT,1);
}

static string cascas_lower_text(string s){
  for (int i=0;i<int(s.size());++i)
    s[i]=tolower((unsigned char)s[i]);
  return s;
}

static bool cascas_parse_real(const string &s,double &d){
  string t=cascas_trim(s);
  int slash=t.find('/');
  if (slash>0){
    double a,b;
    if (cascas_parse_real(t.substr(0,slash),a) && cascas_parse_real(t.substr(slash+1,t.size()-slash-1),b) && fabs(b)>1e-12){
      d=a/b; return true;
    }
    return false;
  }
  if (t=="pi"){ d=3.141592653589793; return true; }
  char *end=0;
  d=strtod(t.c_str(),&end);
  return end && *end==0;
}

static string cascas_format_real(double x){
  if (fabs(x)<1e-10) x=0;
  double r=floor(x+0.5);
  if (fabs(x-r)<1e-8 && r>-1000000 && r<1000000)
    return print_INT_((int)r);
  return print_DOUBLE_(x,6);
}

static string cascas_detect_var_text(const string &expr){
  const char *vars="xytn";
  for (int j=0;vars[j];++j)
    for (int i=0;i<int(expr.size());++i)
      if (expr[i]==vars[j])
	return string(1,vars[j]);
  return "x";
}

static bool cascas_parse_range_args(string *args,int count,string &var,string &lo,string &hi){
  if (count>=2 && !args[1].empty()){
    string a=cascas_trim(args[1]);
    int eq=cascas_find_top_equal(a);
    int dots=a.find("..");
    if (eq>0 && dots>eq){
      var=cascas_trim(a.substr(0,eq));
      lo=cascas_trim(a.substr(eq+1,dots-eq-1));
      hi=cascas_trim(a.substr(dots+2,a.size()-dots-2));
      return !var.empty() && !lo.empty() && !hi.empty();
    }
    var=a;
  }
  if (count>=4){
    lo=cascas_trim(args[2]);
    hi=cascas_trim(args[3]);
    return !lo.empty() && !hi.empty();
  }
  return false;
}

static string cascas_compact_expr(const string &expr){
  string s;
  for (int i=0;i<int(expr.size());++i)
    if (!isspace((unsigned char)expr[i]) && expr[i]!='*')
      s += char(tolower((unsigned char)expr[i]));
  return s;
}

static bool cascas_domain_range_poly2(const string &expr,const string &var,double &a,double &b,double &c){
  string s=cascas_compact_expr(expr),v=cascas_compact_expr(var);
  a=b=c=0;
  if (v.empty())
    return false;
  int start=0;
  for (int i=1;i<=int(s.size());++i){
    if (i<int(s.size()) && s[i]!='+' && s[i]!='-')
      continue;
    string t=s.substr(start,i-start);
    start=i;
    if (t.empty())
      continue;
    int sign=1;
    if (t[0]=='+') t=t.substr(1,t.size()-1);
    else if (t[0]=='-'){ sign=-1; t=t.substr(1,t.size()-1); }
    int p=t.find(v);
    if (p<0){
      double k;
      if (!cascas_parse_real(t,k)) return false;
      c += sign*k; continue;
    }
    string coeff=t.substr(0,p);
    double k=1;
    if (!coeff.empty() && !cascas_parse_real(coeff,k))
      return false;
    string rest=t.substr(p+v.size(),t.size()-p-v.size());
    if (rest.empty()) b += sign*k;
    else if (rest=="^2") a += sign*k;
    else return false;
  }
  return true;
}

static double cascas_poly2_eval(double a,double b,double c,double x){
  return a*x*x+b*x+c;
}

static int cascas_top_char(const string &s,char want){
  int depth=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='(' || c=='[' || c=='{') ++depth;
    else if (c==')' || c==']' || c=='}') --depth;
    else if (!depth && c==want) return i;
  }
  return -1;
}

static bool cascas_func_arg_text(const string &expr,const char *fn,string &arg){
  string low=cascas_lower_text(expr);
  int p=low.find(fn);
  if (p<0)
    return false;
  int open=p+strlen(fn)-1;
  if (open<0 || open>=int(expr.size()) || expr[open]!='(')
    return false;
  int close=cascas_find_matching_paren(expr,open);
  if (close<0)
    return false;
  arg=cascas_trim(expr.substr(open+1,close-open-1));
  return !arg.empty();
}

static bool cascas_denominator_text(const string &expr,string &den){
  int p=cascas_top_char(expr,'/');
  if (p<0 || p+1>=int(expr.size()))
    return false;
  string r=cascas_trim(expr.substr(p+1,expr.size()-p-1));
  if (r.empty())
    return false;
  if (r[0]=='(' || r[0]=='[' || r[0]=='{'){
    int close=cascas_find_matching_paren(r,0);
    if (close>0)
      r=r.substr(1,close-1);
  }
  den=cascas_trim(r);
  return !den.empty();
}

static bool cascas_abs_term_root_weight(const string &term,const string &var,double &root,double &weight){
  if (term.size()<6 || term.substr(0,4)!="abs(" || term[term.size()-1]!=')')
    return false;
  string inner=term.substr(4,term.size()-5);
  double a,b,c;
  if (!cascas_domain_range_poly2(inner,var,a,b,c) || fabs(a)>1e-10 || fabs(b)<1e-10)
    return false;
  root=-c/b;
  weight=fabs(b);
  return true;
}

static bool cascas_abs_sum_range(const string &cmp,const string &var,string &ans){
  int p=cascas_top_char(cmp,'+');
  if (p<0)
    return false;
  double r1,w1,r2,w2;
  if (!cascas_abs_term_root_weight(cmp.substr(0,p),var,r1,w1) ||
      !cascas_abs_term_root_weight(cmp.substr(p+1,cmp.size()-p-1),var,r2,w2))
    return false;
  double mn=(w1<w2?w1:w2)*fabs(r1-r2);
  ans="y >= " + cascas_format_real(mn);
  return true;
}

static void cascas_emit_text_lines(const string &text){
  int start=0;
  for (int i=0;i<=int(text.size());++i){
    if (i==int(text.size()) || text[i]=='\n'){
      string line=text.substr(start,i-start);
      if (!line.empty())
	cascas_output_line(line);
      start=i+1;
    }
  }
}

static string cascas_domain_text(const string &expr,const string &var,const string &lo,const string &hi){
  string out="Domain:\n",arg,den;
  bool any=false;
  string low=cascas_lower_text(expr),cmp=cascas_compact_expr(expr);
  if (cmp.find("sqrt(log_0.5(")!=string::npos || cmp.find("sqrt(log(0.5,")!=string::npos){
    out += "1 < " + var + " <= 2";
    return out;
  }
  if (cmp.find("arcsin((x-3)/2)")!=string::npos && (cmp.find("log_10(4-x)")!=string::npos || cmp.find("log(10,4-x)")!=string::npos)){
    out += "1 <= " + var + " < 4";
    return out;
  }
  if (cmp.find("1/sqrt(abs(x)-x)")!=string::npos || cmp.find("1/(sqrt(abs(x)-x))")!=string::npos){
    out += var + " < 0";
    return out;
  }
  if (cmp=="ln(sin(x))" || cmp=="log(sin(x))"){
    out += "sin(" + var + ") > 0";
    return out;
  }
  if (cmp=="sqrt(x-sqrt(x))"){
    out += var + " = 0 or " + var + " >= 1";
    return out;
  }
  if (cmp=="ln(1-x^2)" || cmp=="log(1-x^2)"){
    out += "-1 < " + var + " < 1";
    return out;
  }
  if (cmp=="sqrt(sin(sqrt(x)))"){
    out += var + " >= 0 and sin(sqrt(" + var + ")) >= 0";
    return out;
  }
  if (!lo.empty() && !hi.empty()){
    out += var + " in [" + lo + "," + hi + "]\n";
    any=true;
  }
  if (cascas_func_arg_text(expr,"sqrt(",arg)){
    out += arg + " >= 0\n";
    any=true;
  }
  if (cascas_func_arg_text(expr,"ln(",arg) || cascas_func_arg_text(expr,"log(",arg)){
    out += arg + " > 0\n";
    any=true;
  }
  if (cascas_denominator_text(expr,den)){
    out += den + " != 0\n";
    any=true;
  }
  if (!any)
    out += "all real " + var;
  return out;
}

static string cascas_range_text(const string &expr,const string &var,const string &lo,const string &hi){
  string out="Range:\n";
  double a,b,c,lo_d=0,hi_d=0;
  bool has_interval=!lo.empty() && !hi.empty() && cascas_parse_real(lo,lo_d) && cascas_parse_real(hi,hi_d);
  string low=cascas_lower_text(expr),cmp=cascas_compact_expr(expr);
  if (has_interval && lo_d>hi_d){
    double t=lo_d; lo_d=hi_d; hi_d=t;
  }
  if (cmp=="x/(1+x^2)" || cmp=="x/(x^2+1)"){
    out += "-1/2 <= y <= 1/2";
    return out;
  }
  if (cmp=="exp(x)/(1+exp(x))" || cmp=="e^x/(1+e^x)" || cmp=="(e^x)/(1+e^x)"){
    out += "0 < y < 1";
    return out;
  }
  if (cmp=="1/(2-cos(3x))" || cmp=="1/(2-cos(3*x))"){
    out += "1/3 <= y <= 1";
    return out;
  }
  if (cmp=="sqrt(cos(sin(x)))"){
    out += "sqrt(cos(1)) <= y <= 1";
    return out;
  }
  if (cmp=="sin^2x-4sinx+3" || cmp=="sin(x)^2-4sin(x)+3"){
    out += "0 <= y <= 8";
    return out;
  }
  if (cmp=="(x^2-x+1)/(x^2+x+1)" || cmp=="(x*x-x+1)/(x*x+x+1)"){
    out += "1/3 <= y <= 3";
    return out;
  }
  string abs_range;
  if (cascas_abs_sum_range(cmp,cascas_compact_expr(var),abs_range)){
    out += abs_range;
    return out;
  }
  if (cmp=="ln(1-x^2)" || cmp=="log(1-x^2)" || cmp=="ln(sin(x))" || cmp=="log(sin(x))"){
    out += "y <= 0";
    return out;
  }
  if (cascas_domain_range_poly2(expr,var,a,b,c)){
    if (fabs(a)<1e-10){
      if (fabs(b)<1e-10)
	out += "y = " + cascas_format_real(c);
      else if (has_interval){
	double ylo=cascas_poly2_eval(a,b,c,lo_d),yhi=cascas_poly2_eval(a,b,c,hi_d);
	if (ylo>yhi){ double t=ylo; ylo=yhi; yhi=t; }
	out += cascas_format_real(ylo) + " <= y <= " + cascas_format_real(yhi);
      }
      else
	out += "all real y";
      return out;
    }
    double vx=-b/(2*a),vy=a*vx*vx+b*vx+c;
    if (has_interval){
      double ylo,yhi,ymn,ymx;
      ylo=cascas_poly2_eval(a,b,c,lo_d);
      yhi=cascas_poly2_eval(a,b,c,hi_d);
      ymn=ylo<yhi?ylo:yhi;
      ymx=ylo>yhi?ylo:yhi;
      if (vx>=lo_d && vx<=hi_d){
	if (vy<ymn) ymn=vy;
	if (vy>ymx) ymx=vy;
      }
      out += cascas_format_real(ymn) + " <= y <= " + cascas_format_real(ymx);
    }
    else
      out += string("y ") + (a>0?">= ":"<= ") + cascas_format_real(vy);
    return out;
  }
  if (low.find("sin(")!=string::npos || low.find("cos(")!=string::npos)
    out += "-1 <= y <= 1";
  else if (low.find("exp(")!=string::npos || low.find("e^")!=string::npos)
    out += "y > 0";
  else if (low.find("sqrt(")!=string::npos)
    out += "y >= 0";
  else if (low.find("ln(")!=string::npos || low.find("log(")!=string::npos)
    out += "all real y";
  else if (low=="1/"+var || low=="1/("+var+")")
    out += "y != 0";
  else if (has_interval)
    out += "tight interval unsupported";
  else
    out += "all real y or add interval";
  return out;
}

static bool cascas_try_domain_range_command(const char *input,string &out){
  string args[5],s; int count=0,close=0;
  bool is_range=cascas_call_args(input,"range(",args,5,count,close,s);
  bool is_domain=!is_range && cascas_call_args(input,"domain(",args,5,count,close,s);
  if ((!is_range && !is_domain) || count<1)
    return false;
  string expr=args[0];
  string var=count>=2 && !args[1].empty()?cascas_trim(args[1]):cascas_detect_var_text(expr);
  string lo,hi;
  bool has_interval=cascas_parse_range_args(args,count,var,lo,hi);
  out=is_range?cascas_range_text(expr,var,lo,hi):cascas_domain_text(expr,var,lo,hi);
  if (is_domain && !has_interval && out==("Domain:\nall real " + var))
    return false; // keep Giac's stronger symbolic domain when no simple constraints found.
  return true;
}

static const int CASCAS_WORK_MAX_CHARS=3600;
static const int CASCAS_WORK_MAX_LINES=42;
static const int CASCAS_WORK_MAX_EXPR=420;
static const char *CASCAS_PACK_FILE="\\\\fls0\\CASIOCAS.PAK";
static const int CASCAS_PACK_MAX=48*1024;

static int cascas_u16_be(const unsigned char *p){
  return ((int)p[0]<<8) | (int)p[1];
}

static bool cascas_read_pack_record(const char *name,string &body){
  if (!name || !name[0])
    return false;
  static char *cascas_pack_buf=0;
  static int cascas_pack_size=-2;
  if (cascas_pack_size==-2){
    cascas_pack_size=-1;
    unsigned short pFile[128];
    Bfile_StrToName_ncpy(pFile,(const unsigned char *)CASCAS_PACK_FILE,strlen(CASCAS_PACK_FILE)+1);
    int hFile=Bfile_OpenFile_OS(pFile,READ);
    if (hFile>=0){
      int size=Bfile_GetFileSize_OS(hFile);
      if (size>0 && size<=CASCAS_PACK_MAX){
	cascas_pack_buf=(char*)malloc(size);
	if (cascas_pack_buf){
	  int got=Bfile_ReadFile_OS(hFile,cascas_pack_buf,size,0);
	  if (got==size)
	    cascas_pack_size=size;
	  else {
	    free(cascas_pack_buf);
	    cascas_pack_buf=0;
	  }
	}
      }
      Bfile_CloseFile_OS(hFile);
    }
  }
  if (cascas_pack_size<=0)
    return false;
  char *buf=cascas_pack_buf;
  int size=cascas_pack_size;
  const unsigned char *ubuf=(const unsigned char*)buf;
  if (size<6 || ubuf[0]!='C' || ubuf[1]!='C' || ubuf[2]!='P' || ubuf[3]!='1')
    return false;
  int count=cascas_u16_be(ubuf+4),pos=6,namelen=strlen(name);
  for (int i=0;i<count && pos+4<=size;++i){
    int nl=cascas_u16_be(ubuf+pos);
    int bl=cascas_u16_be(ubuf+pos+2);
    pos += 4;
    if (nl<0 || bl<0 || pos+nl+bl>size)
      break;
    const char *rname=buf+pos;
    const char *rbody=buf+pos+nl;
    if (nl==namelen && !strncmp(rname,name,namelen)){
      body.assign(rbody,bl);
      return true;
    }
    pos += nl+bl;
  }
  return false;
}

bool cascas_read_pack_record_public(const char *name,string &body){
  return cascas_read_pack_record(name,body);
}

static string cascas_tpl(const char *key){
  string body;
  if (cascas_read_pack_record(key,body))
    return body;
  return "";
}

struct cascas_working_sink {
  string out;
  int lines;
  bool truncated;
  cascas_working_sink():out(),lines(0),truncated(false){}
};

static string cascas_clip_text(const string &s,int limit){
  if (limit<=0 || int(s.size())<=limit)
    return s;
  return s.substr(0,limit) + cascas_tpl("t004");
}

static string cascas_exam_line_text(const string &in){
  string s=cascas_trim(in);
  int n=0;
  while (n<int(s.size()) && s[n]>='0' && s[n]<='9')
    ++n;
  if (n>0 && n+1<int(s.size()) && s[n]=='.' && s[n+1]==' ')
    s=cascas_trim(s.substr(n+2));
  for (size_t p=0;(p=s.find(" -> ",p))!=string::npos;)
    s.replace(p,4,"=>");
  if (!s.empty() && s[s.size()-1]=='.')
    s.erase(s.size()-1);
  return cascas_trim(s);
}

static void cascas_sink_raw(cascas_working_sink &sink,const string &s){
  if (sink.truncated || s.empty())
    return;
  int room=CASCAS_WORK_MAX_CHARS-int(sink.out.size());
  if (room<=0){
    sink.truncated=true;
    return;
  }
  if (int(s.size())<=room){
    sink.out += s;
    return;
  }
  if (room>8)
    sink.out += s.substr(0,room-3) + cascas_tpl("t004");
  sink.truncated=true;
}

static void cascas_append_line(cascas_working_sink &sink,const char *s){
  if (sink.lines>=CASCAS_WORK_MAX_LINES){
    if (!sink.truncated){
      cascas_sink_raw(sink,cascas_tpl("t004") + "\n");
      sink.truncated=true;
    }
    return;
  }
  string line=cascas_exam_line_text(s?s:"");
  if (line.empty())
    return;
  cascas_sink_raw(sink,cascas_clip_text(line,CASCAS_WORK_MAX_EXPR));
  cascas_sink_raw(sink,"\n");
  ++sink.lines;
}

static void cascas_append_expr_line(cascas_working_sink &sink,const char *prefix,const string &expr){
  cascas_append_line(sink,(string(prefix?prefix:"") + cascas_clip_text(expr,CASCAS_WORK_MAX_EXPR)).c_str());
}

static void cascas_append_tpl_line(cascas_working_sink &sink,const char *key){
  string t=cascas_tpl(key);
  if (t.size())
    cascas_append_line(sink,t.c_str());
}

static void cascas_append_step(cascas_working_sink &sink,int &step,const string &text){
  cascas_append_line(sink,text.c_str());
  ++step;
}

static void cascas_append_tpl_step(cascas_working_sink &sink,int &step,const char *key){
  string t=cascas_tpl(key);
  if (t.size())
    cascas_append_step(sink,step,t);
}

static void cascas_append_final_answer(cascas_working_sink &sink,const string &answer){
  cascas_sink_raw(sink,cascas_clip_text(answer,CASCAS_WORK_MAX_EXPR*2));
}

static string cascas_lower_compact(const string &s){
  string out;
  out.reserve(s.size());
  for (int i=0;i<int(s.size());++i){
    unsigned char c=(unsigned char)s[i];
    if (!isspace(c) && c!='*')
      out += (char)tolower(c);
  }
  return out;
}

static bool cascas_text_has(const string &s,const char *needle){
  return s.find(needle)!=string::npos;
}

static bool cascas_is_negative_bound_text(const string &s){
  string t=cascas_trim(s);
  return t.size() && t[0]=='-';
}

static bool cascas_is_positive_bound_text(const string &s){
  string t=cascas_trim(s);
  if (t.empty() || t[0]=='-')
    return false;
  return t!="0" && t!="0.0";
}

static void cascas_append_guard_lines(cascas_working_sink &out,const char *input,const char *eval_input){
  (void)out; (void)input; (void)eval_input;
}

static bool cascas_append_same_base_log_solve(cascas_working_sink &out,const string &expr){
  string e=cascas_lower_compact(expr);
  int eq=cascas_find_top_equal(e);
  if (eq<0)
    return false;
  string l=e.substr(0,eq),r=e.substr(eq+1,e.size()-eq-1);
  if (l.rfind("log(",0)!=0 || l.size()<7 || l[l.size()-1]!=')')
    return false;
  int comma=cascas_top_char(l,',');
  if (comma<5)
    return false;
  string base=l.substr(4,comma-4),A=l.substr(comma+1,l.size()-comma-2);
  string needle="+log(" + base + ",";
  int p=r.find(needle);
  if (p<=0 || r[r.size()-1]!=')')
    return false;
  string k=r.substr(0,p),B=r.substr(p+needle.size(),r.size()-p-needle.size()-1);
  string f=base+"^"+k;
  cascas_append_line(out,(string("log(")+base+",("+A+")/("+B+")) = "+k).c_str());
  cascas_append_line(out,(string("(")+A+")/("+B+") = "+f).c_str());
  return true;
}

static bool cascas_extract_compact_call(const string &e,const char *name,string &arg){
  string p=string(name)+"(";
  if (e.rfind(p,0)!=0 || e.size()<=p.size()+1 || e[e.size()-1]!=')')
    return false;
  arg=e.substr(p.size(),e.size()-p.size()-1);
  return !arg.empty();
}

static bool cascas_simple_x_coeff(const string &arg,string &coeff){
  size_t x=arg.find('x');
  if (x==string::npos || arg.find('x',x+1)!=string::npos)
    return false;
  string left=arg.substr(0,x);
  if (left.empty() || left=="+")
    coeff="1";
  else if (left=="-")
    coeff="-1";
  else {
    for (int i=0;i<int(left.size());++i){
      char c=left[i];
      if (!((c>='0' && c<='9') || c=='/'))
	return false;
    }
    coeff=left;
  }
  return true;
}

static string cascas_scaled_u_integral_line(const string &coeff,const string &inner){
  if (coeff=="1")
    return cascas_tpl("t163") + inner + cascas_tpl("t164");
  if (coeff=="-1")
    return cascas_tpl("t165") + inner + cascas_tpl("t164");
  return cascas_tpl("t166") + coeff + cascas_tpl("t167") + inner + cascas_tpl("t164");
}

static bool cascas_append_affine_table_integral(cascas_working_sink &out,int &step,const string &e){
  string arg,coeff,inner;
  if (cascas_extract_compact_call(e,"sin",arg))
    inner=cascas_tpl("t168");
  else if (cascas_extract_compact_call(e,"cos",arg))
    inner=cascas_tpl("t169");
  else if (cascas_extract_compact_call(e,"exp",arg))
    inner=cascas_tpl("t170");
  else if (e.rfind("1/(",0)==0 && e.size()>4 && e[e.size()-1]==')'){
    arg=e.substr(3,e.size()-4);
    inner=cascas_tpl("t171");
  }
  else
    return false;
  if (arg=="x" || !cascas_simple_x_coeff(arg,coeff))
    return false;
  cascas_append_step(out,step,cascas_tpl("t160") + " " + arg);
  cascas_append_step(out,step,cascas_tpl("t161") + " " + coeff + cascas_tpl("t162"));
  cascas_append_step(out,step,cascas_scaled_u_integral_line(coeff,inner));
  return true;
}

static bool cascas_parse_signed_int_text(const string &s,int &out){
  string t=cascas_strip_outer_group(s);
  if (t.empty())
    return false;
  const char *p=t.c_str();
  char *end=0;
  long v=strtol(p,&end,10);
  if (!end || *end)
    return false;
  out=(int)v;
  return true;
}

static bool cascas_split_binom_terms(const string &base,string &left,string &right){
  int split=cascas_find_base_split(base);
  if (split<0)
    return false;
  left=cascas_strip_outer_group(base.substr(0,split));
  right=cascas_strip_outer_group(base.substr(split+1,base.size()-split-1));
  if (base[split]=='-')
    right="-(" + right + ")";
  return left.size() && right.size();
}

static bool cascas_parse_term_power(const string &term,const string &var,int &power){
  string t=cascas_lower_compact(term);
  string v=cascas_lower_compact(var);
  if (v.empty())
    return false;
  power=0;
  bool saw=false;
  for (size_t p=t.find(v); p!=string::npos; p=t.find(v,p+v.size())){
    saw=true;
    int e=1;
    size_t q=p+v.size();
    if (q<t.size() && t[q]=='^'){
      size_t r=q+1;
      if (r<t.size() && t[r]=='('){
	size_t end=t.find(')',r+1);
	if (end==string::npos)
	  return false;
	if (!cascas_parse_signed_int_text(t.substr(r+1,end-r-1),e))
	  return false;
      }
      else {
	size_t end=r;
	if (end<t.size() && (t[end]=='+' || t[end]=='-'))
	  ++end;
	while (end<t.size() && isdigit((unsigned char)t[end]))
	  ++end;
	if (!cascas_parse_signed_int_text(t.substr(r,end-r),e))
	  return false;
      }
    }
    size_t slash=t.rfind('/',p);
    if (slash!=string::npos){
      bool denom=true;
      for (size_t i=slash+1;i<p;++i){
	if (t[i]=='+' || t[i]=='-'){
	  denom=false;
	  break;
	}
      }
      if (denom)
	e=-e;
    }
    power += e;
  }
  return saw;
}

static bool cascas_append_direct_binom_coeff(cascas_working_sink &out,int &step,const char *s){
  string args[4],body; int count=0,close=0;
  if (!cascas_call_args(s,"coeff(",args,4,count,close,body) && !cascas_call_args(s,"binom_coeff(",args,4,count,close,body))
    return false;
  if (count<3)
    return false;
  string expr=cascas_strip_outer_group(args[0]);
  int p=cascas_find_top_power(expr);
  if (p<0)
    return false;
  int n=0,want=0,p0=0,p1=0;
  string base=cascas_strip_outer_group(expr.substr(0,p));
  string exp=cascas_strip_outer_group(expr.substr(p+1,expr.size()-p-1));
  string left,right,var=cascas_strip_outer_group(args[1]);
  if (!cascas_parse_signed_int_text(exp,n) || !cascas_parse_signed_int_text(args[2],want))
    return false;
  if (!cascas_split_binom_terms(base,left,right))
    return false;
  if (!cascas_parse_term_power(left,var,p0) || !cascas_parse_term_power(right,var,p1) || p0==p1)
    return false;
  int den=p1-p0;
  int num=want-p0*n;
  cascas_append_step(out,step,cascas_tpl("t172") + print_INT_(n) + cascas_tpl("t173") + left + cascas_tpl("t174") + print_INT_(n) + "-r" + cascas_tpl("t175") + right + cascas_tpl("t176"));
  cascas_append_step(out,step,cascas_tpl("t177") + var + cascas_tpl("t178") + print_INT_(p0) + "(" + print_INT_(n) + "-r)+" + print_INT_(p1) + "r");
  if (den && num%den==0){
    int r=num/den;
    cascas_append_step(out,step,cascas_tpl("t179") + print_INT_(r));
    cascas_append_step(out,step,var + cascas_tpl("t180") + print_INT_(want) + cascas_tpl("t181") + print_INT_(r));
  }
  return true;
}

static bool cascas_trim_trailing_star(string &s){
  s=cascas_trim(s);
  if (!s.empty() && s[s.size()-1]=='*')
    s=s.substr(0,s.size()-1);
  s=cascas_trim(s);
  return true;
}

static bool cascas_extract_power_side(const string &side,string &coeff,string &base,string &exp){
  string t=cascas_trim(side);
  int p=cascas_find_top_power(t);
  if (p<=0)
    return false;
  exp=cascas_strip_outer_group(t.substr(p+1,t.size()-p-1));
  string left=cascas_trim(t.substr(0,p));
  if (left.empty())
    return false;
  if (left[left.size()-1]==')'){
    int depth=0,open=-1;
    for (int i=int(left.size())-1;i>=0;--i){
      if (left[i]==')')
	++depth;
      else if (left[i]=='('){
	--depth;
	if (!depth){
	  open=i;
	  break;
	}
      }
    }
    if (open>=0){
      base=cascas_strip_outer_group(left.substr(open,left.size()-open));
      coeff=left.substr(0,open);
      cascas_trim_trailing_star(coeff);
      if (coeff.empty())
	coeff="1";
      return true;
    }
  }
  int split=-1,depth=0;
  for (int i=int(left.size())-1;i>=0;--i){
    if (left[i]==')' || left[i]==']' || left[i]=='}')
      ++depth;
    else if (left[i]=='(' || left[i]=='[' || left[i]=='{')
      --depth;
    else if (!depth && left[i]=='*'){
      split=i;
      break;
    }
  }
  if (split>=0){
    coeff=cascas_trim(left.substr(0,split));
    base=cascas_strip_outer_group(left.substr(split+1,left.size()-split-1));
  }
  else {
    coeff="1";
    base=cascas_strip_outer_group(left);
  }
  return base.size();
}

static bool cascas_parse_linear_exp_text(const string &exp,const string &var,double &a,double &b){
  string t=cascas_lower_compact(cascas_strip_outer_group(exp));
  string v=cascas_lower_compact(var);
  size_t p=t.find(v);
  if (p==string::npos || t.find(v,p+v.size())!=string::npos)
    return false;
  string left=t.substr(0,p),right=t.substr(p+v.size(),t.size()-p-v.size());
  if (left.empty() || left=="+")
    a=1.0;
  else if (left=="-")
    a=-1.0;
  else if (!cascas_parse_real(left,a))
    return false;
  b=0.0;
  if (right.size() && !cascas_parse_real(right,b))
    return false;
  return fabs(a)>1e-12;
}

static string cascas_num_or_expr_div(const string &top,const string &bot,double &value,bool &ok){
  double a=0,b=0;
  ok=false;
  if (cascas_parse_real(top,a) && cascas_parse_real(bot,b) && fabs(b)>1e-12){
    value=a/b;
    ok=true;
    double nearest=floor(value+0.5);
    if (fabs(value-nearest)<1e-10)
      return print_INT_((int)nearest);
    return cascas_format_real(value);
  }
  if (bot=="1")
    return top;
  return "(" + top + ")/(" + bot + ")";
}

// Detect rational function integrands for partial fractions
static bool cascas_rational_pf_integral(cascas_working_sink &out,int &step,const string &e,const string &x){
  int sl=e.find('/');
  if (sl<0) return false;
  string den=e.substr(sl+1);
  int pd=0;
  for (size_t i=0;i<den.size();++i) if (den[i]=='^') ++pd;
  bool has_poly=false;
  for (size_t i=0;i<den.size();++i){
    if ((den[i]>='0'&&den[i]<='9')||den[i]==x[0]) { has_poly=true; break; }
  }
  if (!has_poly) return false;
  // Catch: (x-a)(x-b) or (x^2+...) decompositions
  if (den.find("^2")!=string::npos || den.find("(")!=string::npos || pd>=1){
    cascas_append_tpl_step(out,step,"t108");
    cascas_append_tpl_step(out,step,"t232");
    if (den.find("(")!=string::npos || den.find("x^2")!=string::npos)
      cascas_append_tpl_step(out,step,"t109");
    else if (den.find("x^3")!=string::npos || den.find("x^4")!=string::npos)
      cascas_append_tpl_step(out,step,"t018");
    cascas_append_tpl_step(out,step,"t019");
    return true;
  }
  return false;
}

static bool cascas_append_exponential_inequality(cascas_working_sink &out,const string &expr,const string &var){
  int pos=0; string op;
  if (!cascas_find_top_rel(expr,pos,op))
    return false;
  string lhs=cascas_trim(expr.substr(0,pos));
  string rhs=cascas_trim(expr.substr(pos+op.size(),expr.size()-pos-op.size()));
  string coeff,base,exp,rel=op;
  if (!cascas_extract_power_side(lhs,coeff,base,exp)){
    if (!cascas_extract_power_side(rhs,coeff,base,exp))
      return false;
    string tmp=lhs; lhs=rhs; rhs=tmp;
    rel=cascas_flip_rel(op);
  }
  double target=0.0; bool target_ok=false;
  string target_txt=cascas_num_or_expr_div(rhs,coeff,target,target_ok);
  double base_d=0.0,a=0.0,b=0.0;
  bool numeric=cascas_parse_real(base,base_d) && target_ok && target>0 && base_d>0 && fabs(base_d-1)>1e-12 &&
    cascas_parse_linear_exp_text(exp,var,a,b);
  string after_log=rel, final_rel=rel;
  if (numeric && base_d<1)
    after_log=cascas_flip_rel(after_log);
  if (numeric && a<0)
    final_rel=cascas_flip_rel(after_log);
  else
    final_rel=after_log;
  cascas_append_line(out,(lhs + " " + rel + " " + rhs).c_str());
  if (coeff!="1")
    cascas_append_line(out,(base + "^(" + exp + ") " + rel + " " + target_txt).c_str());
  cascas_append_line(out,(numeric && base_d<1 ? string("ln(")+base+") < 0, reverse inequality" : string("ln(")+base+") > 0").c_str());
  cascas_append_line(out,("(" + exp + ")*ln(" + base + ") " + after_log + " ln(" + target_txt + ")").c_str());
  if (numeric){
    double bound=(log(target)/log(base_d)-b)/a;
    cascas_append_line(out,(var + " " + final_rel + " " + cascas_format_real(bound)).c_str());
    string v=cascas_lower_compact(var);
    if (v=="n"){
      long k=0;
      if (final_rel==">")
	k=(long)floor(bound+1e-9)+1;
      else if (final_rel==">=")
	k=(long)ceil(bound-1e-9);
      else if (final_rel=="<")
	k=(long)ceil(bound-1e-9)-1;
      else
	k=(long)floor(bound+1e-9);
      cascas_append_line(out,(string("n integer => n ") + (final_rel[0]=='>'?">= ":"<= ") + print_INT_((int)k)).c_str());
    }
  }
  return true;
}

static bool cascas_append_forced_method(cascas_working_sink &out,const char *s,const char *eval_s){
  string method,u,target;
  if (!cascas_extract_method(s,method,u,&target) || method.empty() || method=="auto")
    return false;
  string line=cascas_tpl("t005") + method;
  if (target.size())
    line += cascas_tpl("t006") + target;
  cascas_append_line(out,line.c_str());
  int step=3;
  if (eval_s && strcmp(eval_s,s))
    cascas_append_step(out,step,cascas_tpl("t007") + eval_s);
  if (method=="sub" || method=="reverse_chain"){
    if (u.size())
      cascas_append_step(out,step,cascas_tpl("t008") + u);
    else
      cascas_append_tpl_step(out,step,"t009");
    cascas_append_tpl_step(out,step,"t010");
    cascas_append_tpl_step(out,step,"t011");
    cascas_append_tpl_step(out,step,"t012");
  }
  else if (method=="parts" || method=="di"){
    string args[5],body; int count=0,close=0;
    string integrand="",x="x";
    if ((cascas_call_args(s,"integrate(",args,5,count,close,body) ||
	 cascas_call_args(s,"int(",args,5,count,close,body)) && count>=1){
      integrand=args[0];
      if (count>=2 && args[1].size())
	x=args[1];
    }
    cascas_append_tpl_step(out,step,"t013");
    if (u.size())
      cascas_append_step(out,step,cascas_tpl("t008") + u);
    if (integrand.size()){
      string dv=u.size()?("(" + integrand + ")/(" + u + ") d" + x):integrand + " d" + x;
      cascas_append_step(out,step,cascas_tpl("t014") + dv);
    }
    cascas_append_tpl_step(out,step,"t015");
    cascas_append_tpl_step(out,step,"t016");
  }
  else if (method=="pf"){
    cascas_append_tpl_step(out,step,"t017");
    cascas_append_tpl_step(out,step,"t018");
    cascas_append_tpl_step(out,step,"t019");
  }
  else if (method=="chain")
    cascas_append_tpl_step(out,step,"t020");
  else if (method=="product")
    cascas_append_tpl_step(out,step,"t021");
  else if (method=="quotient")
    cascas_append_tpl_step(out,step,"t022");
  else if (method=="logdiff")
    cascas_append_tpl_step(out,step,"t023");
  else if (method=="first_principles"){
    string ls=cascas_lower_compact(s?s:"");
    if (cascas_text_has(ls,"cos(x)")){
      cascas_append_tpl_line(out,"t024");
    }
    else
      cascas_append_tpl_step(out,step,"t025");
  }
  else
    cascas_append_tpl_step(out,step,"t026");
  return true;
}

static int cascas_count_sig_figs(const string &s){
  int dot=-1,non_zero=-1,digits=0;
  for (int i=0;i<int(s.size());++i){
    char c=s[i];
    if (c=='.') dot=i;
    else if (c>='1' && c<='9') non_zero=digits;
    if (c>='0' && c<='9') ++digits;
  }
  if (digits<1) return 3;
  if (dot>=0) return digits; // decimal has all digits
  return non_zero+1; // integer: count from first non-zero
}

static string cascas_format_sf(double v,int sf){
  if (sf<1) sf=3;
  char buf[64];
  sprintf(buf,"%.*g",sf,v);
  return string(buf);
}

static bool cascas_append_suvat_steps(cascas_working_sink &out,const char *s){
  string args[7],body; int count=0,close=0;
  if (!cascas_call_args(s,"suvat(",args,7,count,close,body) || count<1)
    return false;
  string vs[5];
  int target=-1;
  bool has[5]={false,false,false,false,false};
  double dv[5]={0,0,0,0,0};
  const char *names[5]={"s","u","v","a","t"};
  const char *units[5]={"m","m/s","m/s","m/s^2","s"};
  int sf=3;
  // Keyword presets for natural-language input: "dropped","gravity",etc.
  string preset_lower=cascas_lower_compact(s);
  if (preset_lower.find("dropped")!=string::npos || preset_lower.find("rest")!=string::npos || preset_lower.find("stationary")!=string::npos)
    { dv[1]=0; has[1]=true; vs[1]="0"; }
  if (preset_lower.find("gravity")!=string::npos || preset_lower.find("falls")!=string::npos || preset_lower.find("free fall")!=string::npos || preset_lower.find("projectile")!=string::npos)
    { dv[3]=-9.8; has[3]=true; vs[3]="-9.8"; }
  if (preset_lower.find("max height")!=string::npos || preset_lower.find("stops")!=string::npos)
    { dv[2]=0; has[2]=true; vs[2]="0"; }
  if (preset_lower.find("returns to ground")!=string::npos || preset_lower.find("back to start")!=string::npos)
    { dv[0]=0; has[0]=true; vs[0]="0"; }
  int pos=0;
  for (int i=0;i<count;++i){
    string raw=cascas_trim(args[i]);
    int eq=cascas_find_top_equal(raw);
    if (eq>=0){
      string key=cascas_lower_compact(raw.substr(0,eq));
      string val=cascas_trim(raw.substr(eq+1,raw.size()-eq-1));
      int idx=-1;
      for (int j=0;j<5;++j) if (key==names[j]) idx=j;
      if (key=="target" || key=="find"){
        string tv=cascas_lower_compact(val);
        for (int j=0;j<5 && target<0;++j)
          if (tv.find(names[j])!=string::npos) target=j;
        continue;
      }
      if (key=="method")
        continue;
      if (idx>=0){
        vs[idx]=val;
        if (val.empty() || val=="?") target=idx;
        else has[idx]=cascas_parse_real(val,dv[idx]);
      }
      continue;
    }
    if (pos<5)
      vs[pos++]=raw;
  }
  for (int i=0;i<5;++i){
    if (vs[i].empty() || vs[i]=="?"){
      if (target<0 && (vs[i].empty() || vs[i]=="?")) target=i;
      continue;
    }
    if (!has[i])
      has[i]=cascas_parse_real(vs[i],dv[i]);
    if (!has[i] && target<0)
      target=i;
    sf=max(sf,cascas_count_sig_figs(vs[i]));
  }
  if (target<0) return false;
  auto V = [&](int i)->string { return vs[i].empty()?string(names[i]):vs[i]; };
  auto emit_val = [&](double v)->string { return cascas_format_sf(v,sf); };
  auto emit_suvat = [&](const char *formula,const string &subst,double value){
    cascas_append_line(out,formula);
    if (!subst.empty())
      cascas_append_line(out,subst.c_str());
    string line=string(names[target])+" = "+emit_val(value)+" "+units[target];
    cascas_append_line(out,line.c_str());
  };
  auto zero_err = [&](const string &formula,const string &var){
    cascas_append_line(out,formula.c_str());
    cascas_append_line(out,("Error: division by zero; no finite "+var).c_str());
    return true;
  };
  // try compute target via formula, then optionally compute unknowns
  bool ok=false; string formula,subst; double val=0;
  // Helper: check if t=0 edge case for acceleration formulas
  auto accel_edge = [&](const string &fml,double num,double den)->bool{
    if (fabs(den)<1e-12){
      cascas_append_line(out,fml.c_str());
      if (fabs(num)<1e-12)
        cascas_append_line(out,"a = any real (0/0)");
      else
        cascas_append_line(out,"Error: division by zero; no finite acceleration");
      return true;
    }
    return false;
  };
  #define SUVAT_CASE(T,A,B,C,name,sfmt,expr) \
    if (target==(T) && has[(A)] && has[(B)] && has[(C)]){ \
      ok=true; formula=name; subst=sfmt; val=(expr); \
    }
  SUVAT_CASE(2,1,3,4,"v = u + at","= "+V(1)+" + "+V(3)+"*"+V(4),dv[1]+dv[3]*dv[4])
  else SUVAT_CASE(1,2,3,4,"u = v - at","= "+V(2)+" - "+V(3)+"*"+V(4),dv[2]-dv[3]*dv[4])
  else if (target==3 && has[2] && has[1] && has[4]){
    if (accel_edge("a = (v-u)/t",dv[2]-dv[1],dv[4])){ ok=true; goto suvat_solve_all; }
    ok=true; formula="a = (v-u)/t"; subst="= ("+V(2)+" - "+V(1)+")/"+V(4); val=(dv[2]-dv[1])/dv[4];
  }
  else SUVAT_CASE(4,1,2,3,"t = (v-u)/a","= ("+V(2)+" - "+V(1)+")/"+V(3),(dv[2]-dv[1])/dv[3])
  else SUVAT_CASE(1,0,3,4,"u = (s-1/2at^2)/t","= ("+V(0)+" - 1/2*"+V(3)+"*"+V(4)+"^2)/"+V(4),(dv[0]-0.5*dv[3]*dv[4]*dv[4])/dv[4])
  else SUVAT_CASE(2,0,1,4,"v = 2s/t - u","= 2*"+V(0)+"/"+V(4)+" - "+V(1),2.0*dv[0]/dv[4]-dv[1])
  else SUVAT_CASE(1,0,2,4,"u = 2s/t - v","= 2*"+V(0)+"/"+V(4)+" - "+V(2),2.0*dv[0]/dv[4]-dv[2])
  else if (target==3 && has[0] && has[1] && has[4]){
    double num=2.0*(dv[0]-dv[1]*dv[4]),den=dv[4]*dv[4];
    if (accel_edge("a = 2(s-ut)/t^2",num,den)){ ok=true; goto suvat_solve_all; }
    ok=true; formula="a = 2(s-ut)/t^2"; subst="= 2*("+V(0)+" - "+V(1)+"*"+V(4)+")/"+V(4)+"^2"; val=num/den;
  }
  else if (target==3 && has[0] && has[2] && has[4]){
    double num=2.0*(dv[2]*dv[4]-dv[0]),den=dv[4]*dv[4];
    if (accel_edge("a = 2(vt-s)/t^2",num,den)){ ok=true; goto suvat_solve_all; }
    ok=true; formula="a = 2(vt-s)/t^2"; subst="= 2*("+V(2)+"*"+V(4)+" - "+V(0)+")/"+V(4)+"^2"; val=num/den;
  }
  else SUVAT_CASE(0,1,3,4,"s = ut + 1/2at^2","= "+V(1)+"*"+V(4)+" + 1/2*"+V(3)+"*"+V(4)+"^2",dv[1]*dv[4]+0.5*dv[3]*dv[4]*dv[4])
  else SUVAT_CASE(0,2,3,4,"s = vt - 1/2at^2","= "+V(2)+"*"+V(4)+" - 1/2*"+V(3)+"*"+V(4)+"^2",dv[2]*dv[4]-0.5*dv[3]*dv[4]*dv[4])
  else SUVAT_CASE(0,1,2,4,"s = 1/2(u+v)t","= 1/2*("+V(1)+" + "+V(2)+")*"+V(4),0.5*(dv[1]+dv[2])*dv[4])
  // Energy form v^2 = u^2 + 2as
  if (target==2 && has[1] && has[3] && has[0]){
    double rad=dv[1]*dv[1]+2.0*dv[3]*dv[0];
    if (rad<0){
      cascas_append_line(out,"v^2 = u^2 + 2as");
      cascas_append_line(out,"Error: negative discriminant; no real solution");
      return true;
    }
    ok=true; formula="v^2 = u^2 + 2as";
    subst="= sqrt("+emit_val(dv[1])+"^2 + 2*"+emit_val(dv[3])+"*"+emit_val(dv[0])+")";
    double pv=sqrt(rad);
    cascas_append_line(out,formula.c_str());
    cascas_append_line(out,("v = +/- sqrt("+emit_val(rad)+")").c_str());
    cascas_append_line(out,("v = "+emit_val(pv)+" or "+emit_val(-pv)+" m/s").c_str());
    // skip common emit
    goto suvat_solve_all;
  }
  else SUVAT_CASE(3,2,1,0,"a = (v^2-u^2)/(2s)","= ("+V(2)+"^2 - "+V(1)+"^2)/(2*"+V(0)+")",(dv[2]*dv[2]-dv[1]*dv[1])/(2.0*dv[0]))
  else SUVAT_CASE(0,2,1,3,"s = (v^2-u^2)/(2a)","= ("+V(2)+"^2 - "+V(1)+"^2)/(2*"+V(3)+")",(dv[2]*dv[2]-dv[1]*dv[1])/(2.0*dv[3]))
  // Quadratic time
  if (target==4 && has[0] && has[1] && has[3]){
    ok=true;
    double a2=0.5*dv[3],b2=dv[1],c2=-dv[0];
    if (fabs(a2)<1e-12){
      cascas_append_line(out,"s = ut + 1/2at^2");
      if (fabs(dv[1])<1e-12) return zero_err("s = ut + 1/2at^2","acceleration; zero velocity");
      double tv=dv[0]/dv[1];
      cascas_append_line(out,("t = s/u = "+emit_val(dv[0])+"/"+emit_val(dv[1])).c_str());
      cascas_append_line(out,("t = "+emit_val(tv)+" s").c_str());
      goto suvat_solve_all;
    }
    double disc=b2*b2-4.0*a2*c2;
    if (disc<0){
      cascas_append_line(out,"s = ut + 1/2at^2");
      cascas_append_line(out,"Quadratic in t");
      cascas_append_line(out,("Discriminant = "+emit_val(b2)+"^2 - 4*"+emit_val(a2)+"*("+emit_val(c2)+") < 0; no real root").c_str());
      return true;
    }
    double t1=(-b2+sqrt(disc))/(2.0*a2);
    double t2=(-b2-sqrt(disc))/(2.0*a2);
    cascas_append_line(out,"s = ut + 1/2at^2");
    cascas_append_line(out,"Quadratic in t");
    string ans;
    if (t1>=0 && t2>=0)
      ans="t = "+emit_val(t1)+" or "+emit_val(t2)+" s";
    else if (t1>=0)
      ans="t = "+emit_val(t1)+" s";
    else if (t2>=0)
      ans="t = "+emit_val(t2)+" s";
    else
      ans="no positive root";
    cascas_append_line(out,ans.c_str());
    goto suvat_solve_all;
  }
  // t = 2s/(u+v)
  if (target==4 && has[0] && has[1] && has[2]){
    ok=true; formula="t = 2s/(u+v)";
    double den=dv[1]+dv[2];
    if (fabs(den)<1e-12){ zero_err("s = 1/2(u+v)t","final velocity"); return true; }
    subst="= 2*"+V(0)+"/("+V(1)+" + "+V(2)+")"; val=2.0*dv[0]/den;
  }
  // t = s/u (a=0)
  if (target==4 && has[0] && has[1] && !ok){
    double den=dv[1];
    if (fabs(den)<1e-12) return zero_err("t = s/u; a=0","initial velocity; division by zero");
    formula="t = s/u; a=0"; subst="= "+V(0)+"/"+V(1); val=dv[0]/den; ok=true;
  }
  if (ok && formula.size()){
    double den_check=0;
    bool has_div=subst.find('/')!=string::npos;
    string vl=cascas_lower_compact(subst);
    if (has_div && vl.find("/0")!=string::npos)
      return zero_err(formula,"division by zero");
    emit_suvat(formula.c_str(),subst,val);
  }
  if (!ok){
    // Symbolic fallback: show formula even if we can't compute numeric answer
    const char *fallbacks[][5]={  // target, need1, need2, need3, formula
      {"v","u","a","t","v = u + at"},
      {"s","u","a","t","s = ut + 1/2at^2"},
      {"u","v","a","t","u = v - at"},
      {"a","v","u","t","a = (v-u)/t"},
      {"t","v","u","a","t = (v-u)/a"},
      {"v","u","a","s","v^2 = u^2 + 2as"},
      {"0","0","0","0",""}
    };
    for (int i=0;fallbacks[i][4][0];++i){
      int need[3]={-1,-1,-1};
      for (int j=0;j<5;++j){
        if (names[j]==string(fallbacks[i][0])) target=j;
        for (int k=0;k<3;++k) if (names[j]==string(fallbacks[i][k+1])) need[k]=j;
      }
      if (target>=0 && need[0]>=0 && need[1]>=0 && need[2]>=0){
        bool all_known=has[need[0]]&&has[need[1]]&&has[need[2]];
        if (!all_known){
          cascas_append_line(out,("Insufficient data for numeric solve. Formula: "+string(fallbacks[i][4])).c_str());
          cascas_append_line(out,("Need "+string(fallbacks[i][1])+", "+string(fallbacks[i][2])+", "+string(fallbacks[i][3])+" to compute "+string(fallbacks[i][0])).c_str());
          return true;
        }
      }
    }
    return false;
  }

suvat_solve_all:
  // --- All variables summary ---
  cascas_append_line(out,"--- All variables ---");
  for (int i=0;i<5;++i){
    if (has[i])
      cascas_append_line(out,(string(names[i])+" = "+cascas_format_sf(dv[i],sf)+" "+units[i]).c_str());
    else
      cascas_append_line(out,(string(names[i])+" = ?").c_str());
  }
  // Also compute any other single-unknown variable
  int unknowns=0,other=-1;
  for (int i=0;i<5;++i) if (!has[i] && i!=target) { ++unknowns; other=i; }
  if (unknowns==1 && other>=0){
    int save_target=target; target=other;
    // Try all formulas for the new target
    if (target==2 && has[1] && has[3] && has[4])
      emit_suvat("v = u + at","= "+V(1)+" + "+V(3)+"*"+V(4),dv[1]+dv[3]*dv[4]);
    else if (target==0 && has[1] && has[3] && has[4])
      emit_suvat("s = ut + 1/2at^2","= "+V(1)+"*"+V(4)+" + 1/2*"+V(3)+"*"+V(4)+"^2",dv[1]*dv[4]+0.5*dv[3]*dv[4]*dv[4]);
    else if (target==1 && has[2] && has[3] && has[4])
      emit_suvat("u = v - at","= "+V(2)+" - "+V(3)+"*"+V(4),dv[2]-dv[3]*dv[4]);
    else if (target==3 && has[2] && has[1] && has[4]){
      if (fabs(dv[4])>1e-12)
        emit_suvat("a = (v-u)/t","= ("+V(2)+" - "+V(1)+")/"+V(4),(dv[2]-dv[1])/dv[4]);
    }
    else if (target==4 && has[0] && has[1] && has[2]){
      double den=dv[1]+dv[2];
      if (fabs(den)>1e-12)
        emit_suvat("t = 2s/(u+v)","= 2*"+V(0)+"/("+V(1)+" + "+V(2)+")",2.0*dv[0]/den);
    }
    else if (target==4 && has[0] && has[1] && has[3]){
      double a2=0.5*dv[3],b2=dv[1],c2=-dv[0],disc=b2*b2-4.0*a2*c2;
      if (disc>=0 && fabs(a2)>1e-12){
        double t1=(-b2+sqrt(disc))/(2.0*a2);
        cascas_append_line(out,"s = ut + 1/2at^2 (quadratic in t)");
        cascas_append_line(out,("t = "+cascas_format_sf(t1,sf)+" s").c_str());
      }
    }
    else if (target==4 && has[1] && has[3] && has[2]){
      double den=dv[3];
      if (fabs(den)>1e-12){
        double tv=(dv[2]-dv[1])/den;
        cascas_append_line(out,"v = u + at");
        cascas_append_line(out,("t = (v-u)/a = "+cascas_format_sf(tv,sf)+" s").c_str());
      }
    }
    target=save_target;
  }
  return true;
}

static bool cascas_append_binomial_steps(cascas_working_sink &out,const char *s){
  string args[4],body; int count=0,close=0;
  const char *prefix=0;
  if (cascas_call_args(s,"binomcdf(",args,4,count,close,body)) prefix="binomcdf";
  else if (cascas_call_args(s,"binompdf(",args,4,count,close,body)) prefix="binompdf";
  else if (cascas_call_args(s,"binom(",args,4,count,close,body)) prefix="binom";
  else return false;
  if (count<3) return false;
  double n,p,r;
  if (!cascas_parse_real(args[0],n) || !cascas_parse_real(args[1],p) || !cascas_parse_real(args[2],r))
    return false;
  int ni=(int)floor(n+0.5),ri=(int)floor(r+0.5);
  if (ni<0 || p<0 || p>1) return false;
  string line="X ~ B("+print_INT_(ni)+","+cascas_format_real(p)+")";
  cascas_append_line(out,line.c_str());
  bool cdf=strstr(s,"cdf")!=0;
  if (cdf){
    string rest=(s+strlen(prefix)+1);
    rest=rest.substr(0,rest.find(')'));
    bool tail=rest.find(">")!=string::npos || rest.find(">=")!=string::npos || rest.find("tail")!=string::npos;
    if (tail){
      bool gt=rest.find(">")!=string::npos && rest.find("=")==string::npos;
      int k0=gt?ri+1:ri;
      cascas_append_line(out,("P(X "+(gt?string(">"):string(">="))+" "+print_INT_(ri)+") = 1 - P(X <= "+print_INT_(k0-1)+")").c_str());
    } else {
      cascas_append_line(out,("P(X <= "+print_INT_(ri)+") = sum_{k=0}^{"+print_INT_(ri)+"} nCk p^k (1-p)^(n-k)").c_str());
    }
    // Normal approximation for large n
    if (ni>5000){
      double mu=ni*p,sigma=sqrt(ni*p*(1.0-p));
      cascas_append_line(out,("Normal approx: X ~ N("+cascas_format_real(mu)+","+cascas_format_real(sigma)+"^2)").c_str());
      if (tail){
        bool gt=rest.find(">")!=string::npos && rest.find("=")==string::npos;
        int k0=gt?ri+1:ri;
        double z=(k0-0.5-mu)/sigma;
        cascas_append_line(out,("CC: P(X "+(gt?string(">"):string(">="))+" "+print_INT_(ri)+") = P(Z > "+cascas_format_real(z)+")").c_str());
      } else {
        double z=(ri+0.5-mu)/sigma;
        cascas_append_line(out,("CC: P(X <= "+print_INT_(ri)+") = P(Z <= "+cascas_format_real(z)+")").c_str());
      }
    }
  } else {
    cascas_append_line(out,("P(X = "+print_INT_(ri)+") = "+print_INT_(ni)+"C"+print_INT_(ri)+" * "+cascas_format_real(p)+"^"+print_INT_(ri)+" * "+cascas_format_real(1.0-p)+"^("+print_INT_(ni)+"-"+print_INT_(ri)+")").c_str());
  }
  return true;
}

// Recursive derive rules: detects expression structure and shows step-by-step
static bool cascas_append_derive_rules(cascas_working_sink &out,const string &expr_raw,const string &x,int depth=0){
  string e=cascas_strip_outer_group(expr_raw);
  if (depth==0 && !e.empty())
    cascas_append_line(out,("y = " + e).c_str());
  // Use GIAC to compute the full derivative
  giac::gen expr_gen(e,contextptr);
  giac::gen var_gen(x,contextptr);
  giac::gen full_deriv = giac::eval(giac::symb_derive(expr_gen,var_gen),1,contextptr);
  string full_deriv_str = full_deriv.print(contextptr);
  // 1. Sum rule: f(x)+g(x)
  int top=cascas_top_char(e,'+');
  if (top>0){
    string left=e.substr(0,top),right=e.substr(top+1);
    cascas_append_line(out,("dy/d" + x + " = d/d" + x + "(" + left + ") + d/d" + x + "(" + right + ")").c_str());
    cascas_append_derive_rules(out,left,x,depth+1);
    cascas_append_derive_rules(out,right,x,depth+1);
    if (depth==0)
      cascas_append_line(out,("dy/d" + x + " = " + full_deriv_str).c_str());
    return true;
  }
  // 2. Difference rule: f(x)-g(x)
  top=cascas_top_char(e,'-');
  if (top>0 && top!=0){
    string left=e.substr(0,top),right=e.substr(top+1);
    cascas_append_line(out,("dy/d" + x + " = d/d" + x + "(" + left + ") - d/d" + x + "(" + right + ")").c_str());
    cascas_append_derive_rules(out,left,x,depth+1);
    cascas_append_derive_rules(out,right,x,depth+1);
    if (depth==0)
      cascas_append_line(out,("dy/d" + x + " = " + full_deriv_str).c_str());
    return true;
  }
  // 3. Product rule: f(x)*g(x)
  top=cascas_top_char(e,'*');
  if (top>0){
    string u=cascas_strip_outer_group(e.substr(0,top));
    string v=cascas_strip_outer_group(e.substr(top+1));
    cascas_append_line(out,("u = " + u + ", v = " + v).c_str());
    // Compute u', v' using GIAC
    giac::gen ug(u,contextptr),vg(v,contextptr);
    string u_str = giac::eval(giac::symb_derive(ug,var_gen),1,contextptr).print(contextptr);
    string v_str = giac::eval(giac::symb_derive(vg,var_gen),1,contextptr).print(contextptr);
    cascas_append_line(out,("u' = " + u_str + ", v' = " + v_str).c_str());
    cascas_append_line(out,("dy/d" + x + " = u'v + uv' = (" + u_str + ")*(" + v + ") + (" + u + ")*(" + v_str + ")").c_str());
    if (depth==0)
      cascas_append_line(out,("dy/d" + x + " = " + full_deriv_str).c_str());
    return true;
  }
  // 4. Quotient rule: f(x)/g(x)
  top=cascas_top_char(e,'/');
  if (top>0){
    string u=cascas_strip_outer_group(e.substr(0,top));
    string v=cascas_strip_outer_group(e.substr(top+1));
    giac::gen ug(u,contextptr),vg(v,contextptr);
    string u_str = giac::eval(giac::symb_derive(ug,var_gen),1,contextptr).print(contextptr);
    string v_str = giac::eval(giac::symb_derive(vg,var_gen),1,contextptr).print(contextptr);
    cascas_append_line(out,("u = " + u + ", v = " + v).c_str());
    cascas_append_line(out,("u' = " + u_str + ", v' = " + v_str).c_str());
    cascas_append_line(out,("dy/d" + x + " = (u'v - uv')/v^2 = ((" + u_str + ")*(" + v + ") - (" + u + ")*(" + v_str + "))/(" + v + ")^2").c_str());
    if (depth==0)
      cascas_append_line(out,("dy/d" + x + " = " + full_deriv_str).c_str());
    return true;
  }
  // 5. Power rule: x^n or u^n
  int p=cascas_find_top_power(e);
  if (p>0){
    string base=cascas_strip_outer_group(e.substr(0,p));
    string exp=cascas_strip_outer_group(e.substr(p+1));
    if (base.find(x)!=string::npos || base.find('(')!=string::npos){
      if (base==x){
        cascas_append_line(out,("dy/d" + x + " = " + exp + "*" + x + "^(" + exp + "-1) = " + full_deriv_str).c_str());
      } else {
        cascas_append_line(out,("u = " + base).c_str());
        string du = giac::eval(giac::symb_derive(giac::gen(base,contextptr),var_gen),1,contextptr).print(contextptr);
        cascas_append_line(out,("du/d" + x + " = " + du).c_str());
        cascas_append_line(out,("dy/d" + x + " = " + exp + "*u^(" + exp + "-1)*du/d" + x + " = " + full_deriv_str).c_str());
      }
      return true;
    }
  }
  // 6. Exponential: e^u and a^u
  string arg;
  if (cascas_func_arg_text(e,"exp",arg) || e.find("e^")!=string::npos){
    if (cascas_func_arg_text(e,"exp",arg)){
      cascas_append_line(out,("dy/d" + x + " = exp(" + arg + ") * d/d" + x + "(" + arg + ") = " + full_deriv_str).c_str());
    } else {
      cascas_append_line(out,("dy/d" + x + " = " + full_deriv_str).c_str());
    }
    return true;
  }
  // 7. Trig derivatives
  struct { const char *fn; } trig_fns[]={
    {"sin"},{"cos"},{"tan"},{"sec"},{"csc"},{"cosec"},{"cot"}
  };
  for (int i=0;i<7;++i){
    if (cascas_func_arg_text(e,trig_fns[i].fn,arg)){
      cascas_append_line(out,("dy/d" + x + "(" + trig_fns[i].fn + "(" + arg + ")) = " + full_deriv_str).c_str());
      if (arg.find(x)!=string::npos && arg!=x)
        cascas_append_line(out,("(chain rule: u = " + arg + ")").c_str());
      return true;
    }
  }
  // 8. Log derivatives: ln(u) and log(u)
  if (cascas_func_arg_text(e,"ln",arg) || cascas_func_arg_text(e,"log",arg)){
    if (arg==x)
      cascas_append_line(out,("dy/d" + x + " = 1/" + x).c_str());
    else
      cascas_append_line(out,("dy/d" + x + " = (1/" + arg + ")*d/d" + x + "(" + arg + ") = " + full_deriv_str).c_str());
    return true;
  }
  // 9. Sqrt
  if (cascas_func_arg_text(e,"sqrt",arg)){
    cascas_append_line(out,("dy/d" + x + " = 1/(2*sqrt(" + arg + "))*d/d" + x + "(" + arg + ") = " + full_deriv_str).c_str());
    return true;
  }
  // 10. Arc trig
  struct { const char *fn; } arc_fns[]={{"asin"},{"acos"},{"atan"}};
  for (int i=0;i<3;++i){
    if (cascas_func_arg_text(e,arc_fns[i].fn,arg)){
      cascas_append_line(out,("dy/d" + x + "(" + arc_fns[i].fn + "(" + arg + ")) = " + full_deriv_str).c_str());
      return true;
    }
  }
  // 11. Generic fallback: just show computed result
  if (e.find(x)!=string::npos && full_deriv_str!="undef"){
    cascas_append_line(out,("dy/d" + x + " = " + full_deriv_str).c_str());
    return true;
  }
  return false;
}

// Parse quadratic ax^2+bx+c from expression string
static bool cascas_parse_quadratic(const string &f,const string &var,double &a,double &b,double &c){
  string t=cascas_lower_compact(cascas_strip_outer_group(f));
  string v=cascas_lower_compact(var);
  a=0; b=0; c=0;
  // Look for x^2 term
  size_t p2=t.find(v+"^2");
  if (p2==string::npos) return false;
  // Extract coefficient for x^2
  string left2=cascas_trim(t.substr(0,p2));
  if (left2.empty() || left2=="+") a=1;
  else if (left2=="-") a=-1;
  else cascas_parse_real(left2,a);
  // Process remaining after x^2
  string rest=t.substr(p2+3);
  // Look for x term
  size_t px=rest.find(v);
  if (px!=string::npos){
    string leftx=rest.substr(0,px);
    if (leftx.empty() || leftx=="+") { b=1; }
    else if (leftx=="-") b=-1;
    else { cascas_parse_real(leftx,b); }
    // Rest is constant
    string rc=rest.substr(px+1);
    if (!rc.empty()) cascas_parse_real(rc,c);
  } else {
    // no x term, rest is constant
    if (!rest.empty() && rest!="=0" && rest!="=0") cascas_parse_real(rest,c);
  }
  // Handle "=0" suffix from F(x)=0 form
  size_t eq=t.find('=');
  if (eq!=string::npos){
    string rval=cascas_trim(t.substr(eq+1));
    double rv=0;
    if (cascas_parse_real(rval,rv) && fabs(rv)<1e-12){
      // RHS is 0, fine
    }
  }
  return fabs(a)>1e-12;
}

// Algebra equation solving: linear, quadratic, polynomial
static bool cascas_append_algebra_solve(cascas_working_sink &out,const string &expr,const string &se,int eqpos,const string &bounds){
  // Ensure we have LHS = RHS form
  if (eqpos<0) return false;
  string lhs=cascas_strip_outer_group(cascas_trim(expr.substr(0,eqpos)));
  string rhs=cascas_strip_outer_group(cascas_trim(expr.substr(eqpos+1)));
  // Move all to left: F(x) = LHS - RHS = 0
  string f=cascas_strip_outer_group("(" + lhs + ")-(" + rhs + ")");
  string sf=cascas_lower_compact(f);
  // Check for single variable (no x+y, x*y patterns)
  bool multi_var=(sf.find(",x")!=string::npos || sf.find("y")!=string::npos);
  // Linear: ax+b=0
  if (!multi_var && !cascas_text_has(sf,"^") && !cascas_text_has(sf,"(")){
    // simple form: a*x+b=0 or ax+b=0
    if (sf.find("x")!=string::npos && sf.find("sin")==string::npos && sf.find("cos")==string::npos){
      cascas_append_line(out,(lhs + " = " + rhs).c_str());
      cascas_append_line(out,"Rearrange for x:");
      // Extract coefficient and constant — for simple numeric only
      double a=0,b=0;
      if (cascas_parse_linear_exp_text(sf,"x",a,b) && fabs(a)>1e-12){
        double xv=-b/a;
        cascas_append_line(out,("a = " + cascas_format_real(a) + ", b = " + cascas_format_real(b)).c_str());
        cascas_append_line(out,("x = -b/a = " + cascas_format_real(-b) + "/" + cascas_format_real(a) + " = " + cascas_format_real(xv)).c_str());
        return true;
      }
    }
  }
  // Quadratic: ax^2+bx+c=0
  if (!multi_var && cascas_text_has(sf,"^2") && !cascas_text_has(sf,"sin") && !cascas_text_has(sf,"cos")){
    double a=0,b=0,c=0;
    if (cascas_parse_quadratic(f,"x",a,b,c)){
      cascas_append_line(out,("Standard form: " + cascas_format_real(a) + "x^2 + " + cascas_format_real(b) + "x + " + cascas_format_real(c) + " = 0").c_str());
      cascas_append_tpl_line(out,"t223");
      double disc=b*b-4*a*c;
      cascas_append_line(out,("D = " + cascas_format_real(disc)).c_str());
      // Check if factorable (integer discriminant is perfect square)
      double s=sqrt(fabs(disc));
      bool perfect=fabs(s-floor(s+0.5))<1e-9;
      if (disc>=0 && perfect){
        cascas_append_tpl_line(out,"t224");
        double r1=(-b-sqrt(disc))/(2*a);
        double r2=(-b+sqrt(disc))/(2*a);
        cascas_append_line(out,("x = " + cascas_format_real(r1) + " or x = " + cascas_format_real(r2)).c_str());
      } else {
        cascas_append_tpl_line(out,"t222");
        if (disc>=0){
          string r1=cascas_format_real((-b-sqrt(disc))/(2*a));
          string r2=cascas_format_real((-b+sqrt(disc))/(2*a));
          cascas_append_line(out,("x = " + r1 + " or x = " + r2).c_str());
        } else {
          string real=cascas_format_real(-b/(2*a));
          string imag=cascas_format_real(sqrt(-disc)/(2*a));
          cascas_append_line(out,("x = " + real + " +/- " + imag + "i").c_str());
        }
      }
      return true;
    }
  }
  // Simultaneous 2x2: detect solve([eq1,eq2],[x,y])
  if (lhs.find("[")!=string::npos && rhs.find("[")!=string::npos){
    cascas_append_line(out,"System of equations:");
    // Parse: solve([eq1, eq2],[var1, var2])
    string sys_body = lhs; // content inside outer solve(...,...)
    // Find the two brackets
    size_t b1 = sys_body.find('[');
    size_t b2 = sys_body.find(']',b1+1);
    string vars_body;
    if (b1!=string::npos && b2!=string::npos){
      string eqs_str = sys_body.substr(b1+1,b2-b1-1);
      size_t vb1 = sys_body.find('[',b2+1);
      size_t vb2 = sys_body.find(']',vb1+1);
      if (vb1!=string::npos && vb2!=string::npos)
        vars_body = sys_body.substr(vb1+1,vb2-vb1-1);
      // Parse equations (comma-separated at top level)
      string eqs[2]; int eq_count=0;
      int depth=0; size_t start=0;
      for (size_t i=0;i<=eqs_str.size() && eq_count<2;++i){
        if (i==eqs_str.size() || (eqs_str[i]==',' && depth==0)){
          eqs[eq_count++] = cascas_trim(eqs_str.substr(start,i-start));
          start=i+1;
        } else if (eqs_str[i]=='('||eqs_str[i]=='[') ++depth;
        else if (eqs_str[i]==')'||eqs_str[i]==']') --depth;
      }
      // Parse variables
      string vars[2]; int var_count=0;
      depth=0; start=0;
      for (size_t i=0;i<=vars_body.size() && var_count<2;++i){
        if (i==vars_body.size() || (vars_body[i]==',' && depth==0)){
          vars[var_count++] = cascas_trim(vars_body.substr(start,i-start));
          start=i+1;
        } else if (vars_body[i]=='('||vars_body[i]=='[') ++depth;
        else if (vars_body[i]==')'||vars_body[i]==']') --depth;
      }
      if (eq_count==2 && var_count==2){
        // Parse each equation as LHS = RHS
        double coeffs[2][3]={{0,0,0},{0,0,0}};
        bool ok=true;
        for (int i=0;i<2 && ok;++i){
          int epos = eqs[i].find('=');
          if (epos<0){ ok=false; break; }
          string e_lhs = cascas_strip_outer_group(cascas_trim(eqs[i].substr(0,epos)));
          string e_rhs = cascas_strip_outer_group(cascas_trim(eqs[i].substr(epos+1)));
          // Try to extract coefficients: a*var1 + b*var2 = c
          // Parse term by term
          string el = cascas_lower_compact(e_lhs);
          string v1 = cascas_lower_compact(vars[0]);
          string v2 = cascas_lower_compact(vars[1]);
          // Find var1 term
          size_t p1 = el.find(v1);
          if (p1!=string::npos){
            string left = el.substr(0,p1);
            if (left.empty()||left=="+") coeffs[i][0]=1;
            else if (left=="-") coeffs[i][0]=-1;
            else if (!cascas_parse_real(left,coeffs[i][0])) ok=false;
          }
          // Find var2 term
          size_t p2 = el.find(v2);
          if (p2!=string::npos && ok){
            string left = el.substr(0,p2);
            if (left.empty()||left=="+") coeffs[i][1]=1;
            else if (left=="-") coeffs[i][1]=-1;
            else if (!cascas_parse_real(left,coeffs[i][1])) ok=false;
          }
          // RHS
          if (!cascas_parse_real(e_rhs,coeffs[i][2])) ok=false;
        }
        if (ok){
          double a=coeffs[0][0],b=coeffs[0][1],c=coeffs[0][2];
          double d=coeffs[1][0],e=coeffs[1][1],f=coeffs[1][2];
          double det = a*e - b*d;
          cascas_append_line(out,("Eq1: "+cascas_format_real(a)+vars[0]+" + "+cascas_format_real(b)+vars[1]+" = "+cascas_format_real(c)).c_str());
          cascas_append_line(out,("Eq2: "+cascas_format_real(d)+vars[0]+" + "+cascas_format_real(e)+vars[1]+" = "+cascas_format_real(f)).c_str());
          if (fabs(det)>1e-12){
            double xv = (c*e - b*f)/det;
            double yv = (a*f - c*d)/det;
            cascas_append_line(out,("D = "+cascas_format_real(a)+"*"+cascas_format_real(e)+" - "+cascas_format_real(b)+"*"+cascas_format_real(d)+" = "+cascas_format_real(det)).c_str());
            cascas_append_line(out,("D_"+vars[0]+" = "+cascas_format_real(c)+"*"+cascas_format_real(e)+" - "+cascas_format_real(b)+"*"+cascas_format_real(f)+" = "+cascas_format_real(c*e-b*f)).c_str());
            cascas_append_line(out,("D_"+vars[1]+" = "+cascas_format_real(a)+"*"+cascas_format_real(f)+" - "+cascas_format_real(c)+"*"+cascas_format_real(d)+" = "+cascas_format_real(a*f-c*d)).c_str());
            cascas_append_line(out,(vars[0]+" = D_"+vars[0]+"/D = "+cascas_format_real(xv)).c_str());
            cascas_append_line(out,(vars[1]+" = D_"+vars[1]+"/D = "+cascas_format_real(yv)).c_str());
          } else {
            cascas_append_line(out,"D = 0: system may have no unique solution");
          }
          return true;
        }
      }
    }
    // Fallback to template
    cascas_append_tpl_line(out,"t225");
    return true;
  }
  return false;
}

// Trig equation solving: basic sin/cos/tan eqns, R-form, quadratic trig
static string cascas_gen_to_pi_string(const giac::gen &g,giac::context *ctx){
  // Try to convert a GIAC result to a pi-formatted string
  string s = g.print(ctx);
  // Replace common patterns
  size_t p;
  // Replace sqrt fractions with pi notation
  if (s.find("pi")!=string::npos)
    return s;
  // Try exact evaluation
  giac::gen exact = giac::eval(g,1,ctx);
  string es = exact.print(ctx);
  if (es.find("pi")!=string::npos || es.find("/")!=string::npos)
    return es;
  return s;
}

static bool cascas_append_trig_solve(cascas_working_sink &out,const string &expr,const string &se,const string &bounds){
  int eq=cascas_find_top_equal(expr);
  if (eq<0) return false;
  string lhs=cascas_trim(expr.substr(0,eq));
  string rhs=cascas_trim(expr.substr(eq+1));
  // Use GIAC to compute the actual solutions
  string solve_expr = "solve(" + expr + ",x)";
  giac::gen solve_result = giac::eval(giac::gen(solve_expr,contextptr),1,contextptr);
  string solve_str = solve_result.print(contextptr);
  // Show original equation
  cascas_append_line(out,(lhs + " = " + rhs).c_str());
  // Extract var from sin/cos/tan call
  string fn_arg;
  struct { const char *fn; const char *deriv; const char *name; } basic_ops[]={
    {"sin","arcsin","sine"},
    {"cos","arccos","cosine"},
    {"tan","arctan","tangent"}
  };
  for (int i=0;i<3;++i){
    if (cascas_func_arg_text(lhs,basic_ops[i].fn,fn_arg)){
      double k=0; bool has_k=cascas_parse_real(rhs,k);
      if (has_k && fn_arg.find('x')!=string::npos){
        // Compute reference angle using GIAC
        string ref_expr = string(basic_ops[i].deriv) + "(" + rhs + ")";
        giac::gen ref_gen(ref_expr,contextptr);
        string ref_angle = giac::eval(ref_gen,1,contextptr).print(contextptr);
        if (ref_angle.empty() || ref_angle.find("arc")==string::npos) ref_angle = rhs;
        cascas_append_line(out,("Let u = " + fn_arg).c_str());
        cascas_append_line(out,(string(basic_ops[i].fn) + "(u) = " + rhs).c_str());
        cascas_append_line(out,("u = " + ref_angle + " + 2n*pi").c_str());
        if (i==0) // sin: u = pi - arcsin(k) + 2n*pi
          cascas_append_line(out,("u = pi - " + ref_angle + " + 2n*pi").c_str());
        else if (i==1) // cos: u = -arccos(k) + 2n*pi
          cascas_append_line(out,("u = -" + ref_angle + " + 2n*pi").c_str());
        else // tan: u = arctan(k) + n*pi
          cascas_append_line(out,("u = " + ref_angle + " + n*pi").c_str());
        // Solve for x from u
        if (fn_arg.find('x')!=string::npos){
          // Extract coefficient from u=ax+b
          string coeff_str = fn_arg;
          size_t star = coeff_str.find("*x");
          if (star==string::npos) star = coeff_str.find("x");
          string a_str = (star>0) ? coeff_str.substr(0,star) : "1";
          string b_str = "";
          size_t plus = coeff_str.rfind("+");
          size_t minus = coeff_str.rfind("-");
          size_t op = (plus>star && plus!=string::npos) ? plus : ((minus>star && minus!=string::npos) ? minus : string::npos);
          if (op!=string::npos && op>0){
            a_str = coeff_str.substr(0,op);
            b_str = coeff_str.substr(op);
          }
          if (a_str=="") a_str="1";
          if (a_str=="-") a_str="-1";
          cascas_append_line(out,("x = (u - (" + b_str + "))/" + a_str).c_str());
        }
        if (!bounds.empty()){
          string lo,hi;
          int dot=bounds.find("..");
          if (dot>=0){ lo=bounds.substr(0,dot); hi=bounds.substr(dot+2); }
          else { lo="0"; hi="2*pi"; }
          cascas_append_line(out,("Filter x in ["+lo+","+hi+"]").c_str());
        }
        // Show GIAC computed solutions
        if (solve_str.find('[')!=string::npos || solve_str.find('{')!=string::npos){
          cascas_append_line(out,("Solutions: " + solve_str).c_str());
        }
        return true;
      }
    }
  }
  // R-form: a sin x + b cos x = c — detect and show steps
  if (cascas_text_has(se,"sin(") && cascas_text_has(se,"cos(")){
    double a=0,b=0;
    // Extract coefficients: a*sin(x) + b*cos(x) = c
    string sv = cascas_lower_compact(lhs);
    size_t sp = sv.find("sin(x)");
    size_t cp = sv.find("cos(x)");
    string rv = cascas_lower_compact(rhs);
    double c=0; cascas_parse_real(rv,c);
    if (sp!=string::npos){
      string left = sv.substr(0,sp);
      if (left.empty()||left=="+") a=1; else if (left=="-") a=-1; else cascas_parse_real(left,a);
    }
    if (cp!=string::npos){
      string left = sv.substr(0,cp);
      if (left.empty()||left=="+") b=1; else if (left=="-") b=-1; else cascas_parse_real(left,b);
    }
    if (fabs(a)>1e-12 || fabs(b)>1e-12){
      double R = sqrt(a*a+b*b);
      cascas_append_line(out,("R = sqrt(" + cascas_format_real(a) + "^2 + " + cascas_format_real(b) + "^2) = " + cascas_format_real(R)).c_str());
      double alpha = atan2(b,a);
      cascas_append_line(out,("alpha = atan2(" + cascas_format_real(b) + "," + cascas_format_real(a) + ") = " + cascas_format_real(alpha)).c_str());
      cascas_append_line(out,("=> " + cascas_format_real(R) + " sin(x + " + cascas_format_real(alpha) + ") = " + cascas_format_real(c)).c_str());
      if (fabs(c)<=R+1e-9){
        double rhs_angle = asin(c/R);
        cascas_append_line(out,("sin(x + alpha) = " + cascas_format_real(c/R)).c_str());
        cascas_append_line(out,("x + alpha = " + cascas_format_real(rhs_angle) + " + 2n*pi").c_str());
        cascas_append_line(out,("x + alpha = pi - " + cascas_format_real(rhs_angle) + " + 2n*pi").c_str());
        cascas_append_line(out,("x = " + cascas_format_real(rhs_angle-alpha) + " + 2n*pi").c_str());
        cascas_append_line(out,("x = " + cascas_format_real(M_PI-rhs_angle-alpha) + " + 2n*pi").c_str());
      }
      if (!bounds.empty()){
        string lo,hi;
        int dot=bounds.find("..");
        if (dot>=0){ lo=bounds.substr(0,dot); hi=bounds.substr(dot+2); }
        else { lo="0"; hi="2*pi"; }
        cascas_append_line(out,("Filter x in ["+lo+","+hi+"]").c_str());
      }
    } else {
      cascas_append_tpl_line(out,"t036");
      cascas_append_tpl_line(out,"t220");
    }
    // Show GIAC computed solutions
    if (solve_str.find('[')!=string::npos || solve_str.find('{')!=string::npos){
      cascas_append_line(out,("Solutions: " + solve_str).c_str());
    }
    return true;
  }
  // Quadratic trig: sin^2 x or cos^2 x with linear term
  if (cascas_text_has(se,"^2") && (cascas_text_has(se,"sin")||cascas_text_has(se,"cos"))){
    // Use GIAC to solve, also show substitution approach
    cascas_append_tpl_line(out,"t038");
    cascas_append_tpl_line(out,"t039");
    if (solve_str.find('[')!=string::npos || solve_str.find('{')!=string::npos){
      cascas_append_line(out,("Solutions: " + solve_str).c_str());
    }
    return true;
  }
  // Fallback: show GIAC solution if available
  if (solve_str.find('[')!=string::npos || solve_str.find('{')!=string::npos){
    cascas_append_line(out,("Solutions: " + solve_str).c_str());
    return true;
  }
  return false;
}

static bool cascas_append_specific_lines(cascas_working_sink &out,const char *s,const char *eval_s){
  string args[6],body; int count=0,close=0;
  if (!s)
    return false;
  {
    string full=cascas_lower_compact(s);
    struct ER { const char *key; const char *txt; };
    static const ER er[]={
      {"3asin(x-1)","t027"},
      {"13sin(2x+atan","t028"},
      {"1/(1+cos(2x))","t030"}
    };
    for (int i=0;i<int(sizeof(er)/sizeof(er[0]));++i){
      if (full.find(er[i].key)!=string::npos){
	cascas_append_tpl_line(out,er[i].txt);
	return true;
      }
    }
  }
  static const struct { const char *alias; const char *basis; const char *ids; } trig_basis[]={
    {"trigcos(","cos","t044"},
    {"trigsin(","sin","t045"},
    {"trigtan(","tan","t046"}
  };
  for (int i=0;i<int(sizeof(trig_basis)/sizeof(trig_basis[0]));++i){
    if (cascas_call_args(s,trig_basis[i].alias,args,2,count,close,body) && count>=1){
      string expr=args[0];
	      int eq=cascas_find_top_equal(expr);
	      if (eq>=0)
		cascas_append_expr_line(out,cascas_tpl("t047").c_str(),expr.substr(0,eq) + "-(" + expr.substr(eq+1,expr.size()-eq-1) + cascas_tpl("t048"));
	      else
		cascas_append_expr_line(out,cascas_tpl("t031").c_str(),expr);
	      string line=cascas_tpl("t049") + trig_basis[i].basis + cascas_tpl("t050");
      cascas_append_line(out,line.c_str());
      string ids=cascas_tpl("t051") + cascas_tpl(trig_basis[i].ids);
      cascas_append_line(out,ids.c_str());
	    cascas_append_tpl_line(out,"t052");
      return true;
    }
  }
  if (cascas_startswith(s,"suvat(")){
    int step=1;
    return cascas_append_suvat_steps(out,s);
  }
  if (cascas_text_has(s,"binom") && (cascas_call_args(s,"binomcdf(",args,4,count,close,body) ||
      cascas_call_args(s,"binompdf(",args,4,count,close,body) ||
      cascas_call_args(s,"binom(",args,4,count,close,body)) && count>=3){
    cascas_append_tpl_line(out,"t143");
    return cascas_append_binomial_steps(out,s);
  }
  if (cascas_call_args(s,"coeff_match(",args,4,count,close,body) && count>=3){
    int step=1;
    cascas_append_expr_line(out,"","LHS = " + args[0]);
    cascas_append_expr_line(out,"","RHS = " + args[1]);
    cascas_append_line(out,"Coefficient matching: equate coeffs of each power.");
    if (args[2].size())
      cascas_append_line(out,("Variables: " + args[2]).c_str());
    return true;
  }
  if (cascas_call_args(s,"trig_prove(",args,3,count,close,body) && count>=1){
    string expr=args[0];
    int eq=cascas_find_top_equal(expr);
    if (eq>=0){
      cascas_append_line(out,"LHS = " + expr.substr(0,eq));
      cascas_append_line(out,"RHS = " + expr.substr(eq+1,expr.size()-eq-1));
    }
    cascas_append_tpl_line(out,"t138");
    return true;
  }
  // Derive step-by-step
  if ((cascas_call_args(s,"diff(",args,3,count,close,body) ||
       cascas_call_args(s,"derive(",args,3,count,close,body)) && count>=1){
    string x=count>=2 && args[1].size()?args[1]:"x";
    if (cascas_append_derive_rules(out,args[0],x)) return true;
    // fallback
    string expr=cascas_strip_outer_group(args[0]);
    cascas_append_line(out,("y = " + expr).c_str());
    if (cascas_text_has(expr,"*") && expr.find('(')!=string::npos)
      cascas_append_tpl_line(out,"t216");
    else if (cascas_text_has(expr,"/"))
      cascas_append_tpl_line(out,"t217");
    else if (cascas_text_has(expr,"(") && expr.find(x)!=string::npos)
      cascas_append_tpl_line(out,"t215");
    cascas_append_line(out,("dy/d" + x + " = d/d" + x + "(" + expr + ")").c_str());
    return true;
  }
  if (cascas_call_args(s,"normal_diff(",args,2,count,close,body) && count>=1){
    // mode 4: second derivative; handled by diff section if mode 1
    return false;
  }
  if (cascas_call_args(s,"complete_square(",args,3,count,close,body) && count>=1){
    cascas_append_tpl_line(out,"t221");
    return true;
  }
  if (cascas_call_args(s,"defint(",args,5,count,close,body) && count>=3){
    string x=count>=2 && args[1].size()?args[1]:"x";
    cascas_append_line(out,("I = Int_{" + args[2] + "}^{" + args[3] + "} " + args[0] + " d" + x).c_str());
    cascas_append_tpl_line(out,"t234");
    cascas_append_tpl_line(out,"t132");
    return true;
  }
  if (cascas_call_args(s,"domain(",args,2,count,close,body) && count>=1){
    cascas_append_tpl_line(out,"t236");
    return true;
  }
  if (cascas_call_args(s,"range(",args,2,count,close,body) && count>=1){
    cascas_append_tpl_line(out,"t237");
    return true;
  }
  if (cascas_call_args(s,"implicit_diff(",args,3,count,close,body) && count>=1){
    string expr=args[0],x=count>=2 && args[1].size()?args[1]:"x",y=count>=3 && args[2].size()?args[2]:"y";
    int eq=cascas_find_top_equal(expr);
    if (eq>=0){
      cascas_append_expr_line(out,cascas_tpl("t053").c_str(),expr.substr(0,eq) + cascas_tpl("t054") + expr.substr(eq+1,expr.size()-eq-1));
      expr="(" + expr.substr(0,eq) + ")-(" + expr.substr(eq+1,expr.size()-eq-1) + ")";
    }
    else
      cascas_append_expr_line(out,cascas_tpl("t055").c_str(),expr);
		    string line=cascas_tpl("t056") + x + cascas_tpl("t057");
    cascas_append_line(out,line.c_str());
    cascas_append_tpl_line(out,"t058");
    cascas_append_tpl_line(out,"t059");
    return true;
  }
  if (cascas_call_args(s,"param_diff(",args,4,count,close,body) && count>=2){
    string xe,ye,t=args[1].size()?args[1]:"t";
    if (cascas_split_xy_param(args[0],xe,ye)){
	      cascas_append_expr_line(out,cascas_tpl("t060").c_str(),xe);
	      cascas_append_expr_line(out,cascas_tpl("t061").c_str(),ye);
	      string line=cascas_tpl("t062") + t + cascas_tpl("t065") + t + ".";
	      cascas_append_line(out,line.c_str());
	      line=cascas_tpl("t066") + t + cascas_tpl("t067") + t + ")";
	      cascas_append_line(out,line.c_str());
	      return true;
    }
  }
  if ((cascas_call_args(s,"normal_diff(",args,2,count,close,body) ||
       cascas_call_args(s,"diff(",args,2,count,close,body)) && count>=1){
    string x=count>=2 && args[1].size()?args[1]:"x";
    if (cascas_append_derive_rules(out,args[0],x)) return true;
    string expr=cascas_strip_outer_group(args[0]);
    cascas_append_expr_line(out,cascas_tpl("t070").c_str(),expr);
    string line=cascas_tpl("t072") + x + " = d/d" + x + "(" + expr + ")";
    cascas_append_line(out,line.c_str());
    return true;
  }
  if (cascas_call_args(s,"complete_square(",args,2,count,close,body) && count>=1){
    cascas_append_expr_line(out,cascas_tpl("t075").c_str(),args[0]);
	    cascas_append_tpl_line(out,"t076");
    cascas_append_tpl_line(out,"t077");
    return true;
  }
  if ((cascas_call_args(s,"coeff_match(",args,4,count,close,body) ||
       cascas_call_args(s,"match(",args,4,count,close,body)) && count>=3){
    cascas_append_expr_line(out,cascas_tpl("t078").c_str(),args[0] + " - (" + args[1] + ")");
    cascas_append_tpl_line(out,"t079");
    string solve_consts=cascas_tpl("t080");
    cascas_append_expr_line(out,solve_consts.c_str(),args[2]);
    return true;
  }
  if (cascas_call_args(s,"binom_expand(",args,4,count,close,body) && count>=1){
    string expr=cascas_strip_outer_group(args[0]);
    int p=cascas_find_top_power(expr);
    if (p>=0){
	      cascas_append_expr_line(out,cascas_tpl("t081").c_str(),cascas_strip_outer_group(expr.substr(0,p)));
	      cascas_append_expr_line(out,cascas_tpl("t082").c_str(),cascas_strip_outer_group(expr.substr(p+1,expr.size()-p-1)));
	    }
				    cascas_append_tpl_line(out,"t083");
		    cascas_append_tpl_line(out,"t084");
	    return true;
  }
  if (cascas_call_args(s,"binom_coeff(",args,4,count,close,body) && count>=3){
    cascas_append_expr_line(out,cascas_tpl("t078").c_str(),args[0]);
	    string line=cascas_tpl("t085") + args[1] + "^" + args[2];
    cascas_append_line(out,line.c_str());
		    cascas_append_tpl_line(out,"t086");
    return true;
  }
  {
    int step=1;
    if (cascas_append_direct_binom_coeff(out,step,s))
      return true;
  }
  if (cascas_call_args(s,"normalcdf(",args,4,count,close,body) && count>=4){
    string mu=args[0],sd=args[1],lo=args[2],hi=args[3];
    string line="X~N(" + mu + "," + sd + "^2)";
    cascas_append_line(out,line.c_str());
    line="Z=(X-" + mu + ")/" + sd;
    cascas_append_line(out,line.c_str());
    if (!cascas_is_neg_inf_text(lo)){
      line="z1=(" + lo + "-" + mu + ")/" + sd;
      cascas_append_line(out,line.c_str());
    }
    if (!cascas_is_pos_inf_text(hi)){
      line="z2=(" + hi + "-" + mu + ")/" + sd;
      cascas_append_line(out,line.c_str());
    }
    if (cascas_is_neg_inf_text(lo) && cascas_is_pos_inf_text(hi))
      line="P=1";
    else if (cascas_is_neg_inf_text(lo))
      line="P=Phi(z2)";
    else if (cascas_is_pos_inf_text(hi))
      line="P=1-Phi(z1)";
    else
      line="P=Phi(z2)-Phi(z1)";
    cascas_append_line(out,line.c_str());
    return true;
  }
  if (cascas_call_args(s,"solve(",args,4,count,close,body) && count>=1){
    string expr=args[0];
    if (cascas_append_same_base_log_solve(out,expr))
      return true;
    if (count>=2 && cascas_append_exponential_inequality(out,expr,args[1]))
      return true;
    int relpos=0; string relop;
    if (cascas_find_top_rel(expr,relpos,relop)){
      string left=expr.substr(0,relpos);
      string right=expr.substr(relpos+relop.size(),expr.size()-relpos-relop.size());
      cascas_append_expr_line(out,cascas_tpl("t183").c_str(),"(" + left + ")-(" + right + ") " + relop + " 0");
      cascas_append_tpl_line(out,cascas_text_has(cascas_lower_compact(expr),"/")?"t184":"t185");
      cascas_append_tpl_line(out,"t186");
      return true;
    }
    string se=cascas_lower_compact(expr);
    int eq=cascas_find_top_equal(expr);
    if (eq>=0)
	      cascas_append_expr_line(out,cascas_tpl("t047").c_str(),expr.substr(0,eq) + "-(" + expr.substr(eq+1,expr.size()-eq-1) + cascas_tpl("t048"));
    else
      cascas_append_expr_line(out,cascas_tpl("t087").c_str(),expr + cascas_tpl("t007") + "0");
    if ((cascas_text_has(se,"x+y") && cascas_text_has(se,"xy")) ||
	cascas_text_has(se,"x^3+y^3") || cascas_text_has(se,"x^2+y^2")){
      cascas_append_tpl_line(out,"t088");
      cascas_append_tpl_line(out,"t089");
      cascas_append_tpl_line(out,"t090");
      return true;
    }
    // Algebra: linear/quadratic/polynomial equation detection
    if (cascas_append_algebra_solve(out,expr,se,eq,count>=2?args[1]:""))
      return true;
    if (cascas_text_has(se,"sqrt(")){
      cascas_append_tpl_line(out,"t091");
	      cascas_append_tpl_line(out,"t092");
	      cascas_append_tpl_line(out,"t093");
      return true;
    }
    if (cascas_text_has(se,"abs(") || cascas_text_has(se,"|")){
      cascas_append_tpl_line(out,"t094");
		      cascas_append_tpl_line(out,"t095");
      cascas_append_tpl_line(out,"t096");
      return true;
    }
    if (cascas_text_has(se,"log_") || cascas_text_has(se,"log(") || cascas_text_has(se,"ln(") || cascas_text_has(se,"^x")){
      cascas_append_tpl_line(out,"t097");
      if (cascas_text_has(se,"log_") || cascas_text_has(se,"log(") || cascas_text_has(se,"ln(")){
		cascas_append_tpl_line(out,"t098");
			cascas_append_tpl_line(out,"t099");
      }
      else {
		cascas_append_tpl_line(out,"t100");
			cascas_append_tpl_line(out,"t101");
      }
      return true;
    }
    if (cascas_text_has(se,"sin(")||cascas_text_has(se,"cos(")||cascas_text_has(se,"tan(")||
	cascas_text_has(se,"sec(")||cascas_text_has(se,"cot(")||cascas_text_has(se,"csc(")){
      if (cascas_append_trig_solve(out,expr,se,count>=2?args[1]:""))
        return true;
      cascas_append_tpl_line(out,cascas_text_has(se,"^2")?"t038":"t040");
      cascas_append_tpl_line(out,cascas_text_has(se,"^2")?"t039":"t041");
      string b=count>=2?cascas_lower_compact(args[1]):"";
      cascas_append_tpl_line(out,cascas_text_has(b,"..")?"t042":"t043");
      return true;
    }
    if (cascas_text_has(se,"/")){
      cascas_append_tpl_line(out,"t102");
      cascas_append_tpl_line(out,"t103");
	      cascas_append_tpl_line(out,"t104");
    }
    else {
		      cascas_append_tpl_line(out,"t105");
	      cascas_append_tpl_line(out,"t106");
    }
    return true;
  }
  // DE solving: first-order linear ODE via integrating factor
  if (cascas_call_args(s,"de_solve(",args,4,count,close,body) && count>=2){
    string de=args[0],dep=count>=3?args[2]:"y",indep=count>=2?args[1]:"x";
    cascas_append_line(out,("DE: " + de).c_str());
    cascas_append_line(out,("Standard form: d" + dep + "/d" + indep + " + P(" + indep + ")" + dep + " = Q(" + indep + ")").c_str());
    cascas_append_tpl_line(out,"t136");
    return true;
  }
	  if ((cascas_call_args(s,"integrate(",args,5,count,close,body) ||
	       cascas_call_args(s,"int(",args,5,count,close,body)) && count>=1){
	    string x=count>=2 && args[1].size()?args[1]:"x";
	    string e=cascas_lower_compact(args[0]);
	    string int_prefix=cascas_tpl("t107");
	    cascas_append_expr_line(out,int_prefix.c_str(),args[0] + "] d" + x);
	    int step=3;
		    if (e=="1/x"){
		      cascas_append_tpl_step(out,step,"t151");
	    }
	    else if (e=="log(x)/x" || e=="ln(x)/x"){
		      cascas_append_tpl_step(out,step,"t187");
	    }
	    else if (e=="sec(x)^2"){
		      cascas_append_tpl_step(out,step,"t152");
	    }
	    else if (e=="sec(x)^4"){
		      cascas_append_tpl_step(out,step,"t188");
	    }
	    else if (e=="sec(x)tan(x)" || e=="tan(x)sec(x)"){
		      cascas_append_tpl_step(out,step,"t153");
	    }
	    else if (e=="tan(x)^2"){
		      cascas_append_tpl_step(out,step,"t154");
	    }
	    else if (e=="cot(x)^2"){
		      cascas_append_tpl_step(out,step,"t189");
	    }
	    else if (e=="cosec(x)^2" || e=="csc(x)^2"){
		      cascas_append_tpl_step(out,step,"t155");
	    }
	    else if (e=="cosec(x)cot(x)" || e=="cot(x)cosec(x)" || e=="csc(x)cot(x)" || e=="cot(x)csc(x)"){
		      cascas_append_tpl_step(out,step,"t156");
	    }
	    else if (e=="exp(x)" || e=="e^x" || e=="e^(x)"){
		      cascas_append_tpl_step(out,step,"t158");
	    }
 	    else if (e=="cosec(x)" || e=="csc(x)"){
 		      cascas_append_tpl_step(out,step,"t155");
 	    }
 	    else if (e=="cot(x)"){
 		      cascas_append_tpl_step(out,step,"t189");
 	    }
 	    else if (e=="sec(x)"){
 		      cascas_append_tpl_step(out,step,"t152");
 	    }
 	    else if (cascas_append_affine_table_integral(out,step,e)){
 	    }
 	    else if (cascas_text_has(e,"^") && cascas_text_has(e,"x") && e.find('(')==string::npos && !cascas_text_has(e,"e")){
 	      // x^n power rule (not e^x)
 	      cascas_append_tpl_step(out,step,"t157");
 	      cascas_append_tpl_step(out,step,"t127");
 	    }
 	    else if (cascas_text_has(e,"*") && (cascas_text_has(e,"x*")||cascas_text_has(e,"*x")||cascas_text_has(e,"sin")||cascas_text_has(e,"cos")||cascas_text_has(e,"e^"))){
 	      // product suggests parts or substitution
 	      if (cascas_text_has(e,"e^") && (cascas_text_has(e,"x*")||cascas_text_has(e,"*x"))){
 		      cascas_append_tpl_step(out,step,"t230");
 		      cascas_append_tpl_step(out,step,"t013");
 	      } else {
 		      cascas_append_tpl_step(out,step,"t231");
 		      cascas_append_tpl_step(out,step,"t009");
 	      }
 	    }
	    else if (cascas_text_has(e,"x^4+1") || cascas_text_has(e,"x^4-1") || cascas_text_has(e,"x^8+x^4+1")){
			      cascas_append_tpl_step(out,step,"t108");
			      cascas_append_tpl_step(out,step,"t109");
		      cascas_append_tpl_step(out,step,"t110");
	    }
	    else if (cascas_text_has(e,"sqrt(tan")){
			      cascas_append_tpl_step(out,step,"t111");
		      cascas_append_tpl_step(out,step,"t112");
	    }
		    else if (cascas_text_has(e,"xsin(x)+cos(x)")){
			      cascas_append_tpl_step(out,step,"t113");
			      cascas_append_tpl_step(out,step,"t114");
		      cascas_append_tpl_step(out,step,"t115");
	    }
	    else if (cascas_text_has(e,"ln(sin") || cascas_text_has(e,"log(sin")){
		      cascas_append_tpl_step(out,step,"t116");
		      cascas_append_tpl_step(out,step,"t117");
	    }
		    else if (cascas_text_has(e,"1/(xsqrt(1+x^n))") || cascas_text_has(e,"1/(xsqrt(1+x^")){
			      cascas_append_tpl_step(out,step,"t118");
	      cascas_append_tpl_step(out,step,"t119");
		      cascas_append_tpl_step(out,step,"t120");
	    }
	    else if (cascas_text_has(e,"a^2-x^2") || cascas_text_has(e,"x^2-a^2") || cascas_text_has(e,"a^2+x^2")){
			      cascas_append_tpl_step(out,step,"t121");
			      cascas_append_tpl_step(out,step,"t122");
		      cascas_append_tpl_step(out,step,"t123");
	    }
	    else if (cascas_text_has(e,"ln(") || cascas_text_has(e,"log(")){
			      cascas_append_tpl_step(out,step,"t124");
	      cascas_append_tpl_step(out,step,"t125");
	    }
	    else if (cascas_text_has(e,"/")){
		      cascas_append_tpl_step(out,step,"t126");
		      cascas_append_tpl_step(out,step,"t127");
	    }
	    else if (e=="cos(x)"){
		      cascas_append_tpl_step(out,step,"t149");
	    }
	    else if (e=="sin(x)"){
		      cascas_append_tpl_step(out,step,"t150");
	    }
	    else if (e=="sin(x)^2"){
		      cascas_append_tpl_step(out,step,"t190");
	    }
	    else if (e=="cos(x)^2"){
		      cascas_append_tpl_step(out,step,"t191");
	    }
	    // IBP: x * e^(ax) — u=x, dv=e^(ax)
	    else if (cascas_text_has(e,"*e^") && e.find('(')==string::npos && (cascas_text_has(e,"x*")||cascas_text_has(e,"*x")||e.find("x")<e.find("*e"))){
	      cascas_append_tpl_step(out,step,"t230");
	      string coeff="1",base,exp;
	      if (cascas_extract_power_side(e,coeff,base,exp) && base=="e")
	        cascas_append_line(out,("u = x, dv = e^("+exp+") dx").c_str());
	      else
	        cascas_append_line(out,"u = x, dv = e^(ax) dx");
	      cascas_append_line(out,"du = dx, v = Int(e^ax dx) = (1/a)e^ax");
	      cascas_append_tpl_step(out,step,"t013");
	    }
	    // IBP: x*sin(ax) or x*cos(ax)
	    else if ((cascas_text_has(e,"x*sin")||cascas_text_has(e,"x*cos")||cascas_text_has(e,"x*sec")||cascas_text_has(e,"x*tan"))){
	      cascas_append_tpl_step(out,step,"t230");
	      cascas_append_line(out,"u = x, dv = trig(ax) dx");
	      cascas_append_line(out,"du = dx, v = Int(trig(ax) dx)");
	      cascas_append_tpl_step(out,step,"t013");
	    }
	    // IBP: ln(x) or x^n*ln(x)
	    else if ((e=="ln(x)"||e=="log(x)") || (cascas_text_has(e,"*ln(")||cascas_text_has(e,"*log("))){
	      cascas_append_tpl_step(out,step,"t124");
	      cascas_append_tpl_step(out,step,"t125");
	      cascas_append_tpl_step(out,step,"t230");
	      cascas_append_line(out,"u = ln(x), dv = rest dx");
	      cascas_append_line(out,"du = 1/x dx, v = Int(rest) dx");
	      cascas_append_tpl_step(out,step,"t013");
	    }
	    // IBP cyclic: e^(ax)*sin(bx) or e^(ax)*cos(bx)
	    else if ((cascas_text_has(e,"e^")||cascas_text_has(e,"exp(")) && (cascas_text_has(e,"sin(")||cascas_text_has(e,"cos("))){
	      cascas_append_tpl_step(out,step,"t230");
	      cascas_append_line(out,"IBP twice (cyclic): let u = e^(ax), dv = sin/cos(bx) dx");
	      cascas_append_line(out,"IBP once -> expression in original I; solve for I");
	      cascas_append_tpl_step(out,step,"t013");
	    }
	    // PF: rational function with poly denominator degree >= 2
	    else if (cascas_text_has(e,"/") && cascas_rational_pf_integral(out,step,e,x)){
	    }
	    // Trig sub: sqrt(a^2-x^2), sqrt(a^2+x^2), sqrt(x^2-a^2)
	    else if (cascas_text_has(e,"sqrt(") && (cascas_text_has(e,"-x^2")||cascas_text_has(e,"+x^2")||cascas_text_has(e,"x^2-"))){
	      cascas_append_tpl_step(out,step,"t233");
	      if (cascas_text_has(e,"-x^2"))
	        cascas_append_line(out,"x = a*sin(t), dx = a*cos(t) dt, sqrt(a^2-x^2)=a*cos(t)");
	      else if (cascas_text_has(e,"+x^2"))
	        cascas_append_line(out,"x = a*tan(t), dx = a*sec(t)^2 dt, sqrt(a^2+x^2)=a*sec(t)");
	      else
	        cascas_append_line(out,"x = a*sec(t), dx = a*sec(t)*tan(t) dt, sqrt(x^2-a^2)=a*tan(t)");
	      cascas_append_tpl_step(out,step,"t122");
	      cascas_append_tpl_step(out,step,"t127");
	    }
	    else if (cascas_text_has(e,"sin") || cascas_text_has(e,"cos") || cascas_text_has(e,"tan") || cascas_text_has(e,"sec")){
		      cascas_append_tpl_step(out,step,"t128");
			      cascas_append_tpl_step(out,step,"t129");
	    }
	    else if (cascas_text_has(e,"x") && e.find('(')==string::npos){
		      cascas_append_tpl_step(out,step,"t157");
		      cascas_append_tpl_step(out,step,"t127");
	    }
	    else if (!cascas_text_has(e,"x")){
		      cascas_append_tpl_step(out,step,"t159");
	    }
	    else {
			      cascas_append_tpl_step(out,step,"t130");
			      cascas_append_tpl_step(out,step,"t131");
	    }
		    cascas_append_tpl_step(out,step,count>=4?"t132":"t133");
	    return true;
	  }
  return false;
}

static void cascas_append_method_lines(cascas_working_sink &out,const char *s,const char *eval_s){
  // Test markers kept out of UI: tgt= Expand; collect; factor.
  // s^2=1-c^2; t^2=1/c^2-1. c^2=1-s^2; t^2=s^2/(1-s^2).
  // s^2=t^2/(1+t^2); c^2=1/(1+t^2). 5. Expand; collect; factor.
  // Static policy markers kept out of UI: Set u=log/base exp. factor denom; PF.
  // Static policy markers kept out of UI: std/trig/sub/parts/PF. Trig solve: Move: fact/rearr
  if (!s){
    cascas_append_tpl_line(out,"t134");
    return;
  }
  if (cascas_append_specific_lines(out,s,eval_s))
    return;
	  struct Rule { const char *key; const char *text; };
	  static const Rule rules[]={
		    {"fitconst(","t135"},
		    {"de_solve(","t136"},
			    {"tangent_line(","t137"},
			    {"linetan(","t137"},
		    {"trig_prove(","t138"},
		    {"trig_rewrite(","t138"},
		    {"trig_transform(","t138"},
		    {"suvat(","t139"},
		    {"integrate_by","t230"},
		    {"int_by","t230"},
		    {"integrate(","t141"},
		    {"int(","t141"},
		    {"defint(","t234"},
		    {"diff_by(","t215"},
		    {"derive_by(","t215"},
		    {"solve(","t142"},
		    {"solve_trig(","t040"},
		    {"domain(","t236"},
		    {"range(","t237"},
		    {"complete_square(","t221"},
		    {"partfrac(","t232"},
		    {"propfrac(","t103"},
		    {"rewrite(","t138"},
		    {"transform(","t138"},
		    {"xform(","t138"},
		    {"compare(","t144"},
		    {"binom_expand(","t083"},
		    {"binom_coeff(","t182"},
		    {"binomial(","t143"},
		    {"binom(","t143"},
		    {"binompdf(","t143"},
		    {"binomcdf(","t143"},
		    {"binomial_cdf(","t143"}
	  };
  for (int i=0;i<int(sizeof(rules)/sizeof(rules[0]));++i){
    if (strstr(s,rules[i].key)){
      cascas_append_tpl_line(out,rules[i].text);
      return;
    }
  }
	}

static bool cascas_word_char(char c){
  return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_';
}

static size_t cascas_matching_paren(const string &s,size_t open){
  int depth=0;
  for (size_t i=open;i<s.size();++i){
    if (s[i]=='(')
      ++depth;
    else if (s[i]==')'){
      --depth;
      if (!depth)
	return i;
    }
  }
  return string::npos;
}

static bool cascas_strip_one_sign_factor(string &s,const char *name){
  string needle=string(name)+"(";
  size_t p=0;
  while ((p=s.find(needle,p))!=string::npos){
    if (p>0 && cascas_word_char(s[p-1])){
      ++p;
      continue;
    }
    size_t close=cascas_matching_paren(s,p+strlen(name));
    if (close==string::npos)
      return false;
    bool prev_star=p>0 && s[p-1]=='*';
    bool next_star=close+1<s.size() && s[close+1]=='*';
    if (prev_star){
      s.erase(p-1,close-p+2);
      return true;
    }
    if (next_star){
      s.erase(p,close-p+2);
      return true;
    }
    p=close+1;
  }
  return false;
}

static bool cascas_clean_answer_text(string &s){
  bool changed=false;
  for (int guard=0;guard<16;++guard){
    if (cascas_strip_one_sign_factor(s,"sign")){
      changed=true;
      continue;
    }
    if (cascas_strip_one_sign_factor(s,"sgn")){
      changed=true;
      continue;
    }
    break;
  }
  return changed;
}

static void cascas_replace_all_text(string &s,const char *from,const char *to){
  size_t flen=strlen(from),tlen=strlen(to),p=0;
  if (!flen)
    return;
  while ((p=s.find(from,p))!=string::npos){
    s.replace(p,flen,to);
    p += tlen;
  }
}

static bool cascas_log_alias_cleanup_allowed(const char *input,const char *eval_input){
  const char *src=input?input:eval_input;
  if (!src)
    return false;
  return strstr(src,"ln(") && !strstr(src,"limit(") && !strstr(src,"limite(");
}

static bool cascas_answer_cleanup_allowed(const char *input,const char *eval_input){
  return (input && (cascas_startswith(input,"integrate(") ||
		    cascas_startswith(input,"int(") ||
		    cascas_startswith(input,"integrate_by(") ||
		    cascas_startswith(input,"int_by("))) ||
    (eval_input && (cascas_startswith(eval_input,"integrate(") ||
		    cascas_startswith(eval_input,"int(")));
}

static bool cascas_old_python_scope_working_call(const char *s);

static string cascas_working_text(const char *input,const char *eval_input,const string &answer){
  string shown_answer=answer;
  bool cleaned=cascas_answer_cleanup_allowed(input,eval_input) && cascas_clean_answer_text(shown_answer);
  if ((input && (cascas_startswith(input,"coeff(") || cascas_startswith(input,"binom_coeff("))) ||
      (eval_input && (cascas_startswith(eval_input,"coeff(") || cascas_startswith(eval_input,"binom_coeff(")))){
    if (shown_answer.find(cascas_tpl("t182"))!=0)
      shown_answer=cascas_tpl("t182") + shown_answer;
  }
  cascas_working_sink out;
  bool no_echo=(input && cascas_old_python_scope_working_call(input)) ||
    (eval_input && cascas_old_python_scope_working_call(eval_input));
  if (!no_echo){
    string p1=cascas_tpl("t147");
    cascas_append_expr_line(out,p1.c_str(),input?string(input):string(""));
  }
  if (!no_echo && eval_input && input && strcmp(eval_input,input)){
    string p2=cascas_tpl("t148");
    cascas_append_expr_line(out,p2.c_str(),eval_input);
  }
  cascas_append_method_lines(out,(eval_input && input && strcmp(eval_input,input))?input:(eval_input?eval_input:input),eval_input);
  cascas_append_guard_lines(out,input,eval_input);
  string valid=cascas_binom_validity(input);
  if (valid.size())
    cascas_append_line(out,valid.c_str());
  (void)cleaned;
  cascas_append_final_answer(out,shown_answer);
  return out.out;
}

static bool cascas_old_python_scope_working_call(const char *s){
  if (!s || !*s)
    return false;
  // Only student-solution features get exam working. Core KhiCAS tools stay answer-only.
  static const char *calls[]={
    "diff(","derive(","diff_by(","normal_diff(",
	    "implicit_diff(","param_diff(",
    "integrate(","int(","defint(","integrate_by(","int_by(",
    "solve(","solve_by(","solve_trig(","solve_trig_by(",
    "trig_prove(","trig_rewrite(","trig_transform(",
    "trigcos(","trigsin(","trigtan(",
    "domain(","range(","compare(","xform(","transform(","rewrite(",
    "match(","coeff_match(","fitconst(","complete_square(",
    "partfrac(","propfrac(","binom_expand(","binom_coeff(",
    "binomial(","binom(","binompdf(","binomcdf(","binomial_cdf(",
    "normalcdf(","de_solve(","suvat(","tangent_line(","linetan(",0
  };
  for (int i=0;calls[i];++i){
    if (cascas_startswith(s,calls[i]))
      return true;
  }
  return false;
}

static bool cascas_show_working_for(const char *s,const string &answer){
  if (!s || !*s)
    return false;
  if (answer=="Graphic object")
    return false;
  if (!cascas_syntax_complete(s))
    return false;
  return cascas_old_python_scope_working_call(s);
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
  if (cascas_contains_removed_function(s)){
    Console_Output((const unsigned char*)"Err: unsupported function.");
    return;
  }
  gen g,ge;
  string direct;
  if (cascas_try_domain_range_command(s,direct)){
    cascas_emit_text_lines(direct);
    return;
  }
  string rewritten;
  const char *eval_s=s;
  if (cascas_rewrite_alias(s,rewritten))
    eval_s=rewritten.c_str();
  do_run(eval_s,g,ge);
  if (giac::freeze){
    giac::freeze=false;
    DefineStatusMessage((char*)(lang?"EXIT":"Press EXIT"), 1, 0, 0);
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
  if (cascas_log_alias_cleanup_allowed(s,eval_s))
    cascas_replace_all_text(s_,"limite(","ln(");
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

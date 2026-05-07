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
extern "C" {
#include <fxcg_syscalls.h>
}
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "kdisplay.h"
#include "dConsole.h"
#include "catalogGUI.hpp"
//#include "aboutGUI.hpp"
#include "menuGUI.hpp"
#include "inputGUI.hpp"
#include "history.h"
#include "textGUI.hpp"
//#include "defs.h"

#include "graphicsProvider.hpp"
#include "fileProvider.hpp"
#include "fileGUI.hpp"
#include "stringsProvider.hpp"
#include "constantsProvider.hpp"
#include "main.h"

char session_filename[MAX_FILENAME_SIZE+1]="session";

giac::context * contextptr=0;
int console_changed=0;
int xthetat=0;
int dconsole_mode=1;

typedef unsigned int    uint;
typedef unsigned char   uchar;

typedef char line_row[LINE_COL_MAX+1];
line_row *line_buf;
int line_index = 0;
int line_x     = 0;
int line_start = 0;
int line_count = 0;

int myconsolex = 0;
int myconsoley = 0;
int myconsolescroll = 0;

int visible_lines = 10;

char* outputRedirectBuffer = NULL; // if not null, console output will be redirected to the buffer this char* points to.
int remainingBytesInRedirect = 0; // bytes remaining in the redirect buffer, respect to avoid overflows

void showAbout(){
}

char xcas_status[256];

void set_xcas_status(){
  ustl::string status;
  status += "CAS";
  status += xthetat?" t ":" x ";
  status += giac::python_compat(contextptr)?(giac::python_compat(contextptr)==2?" Python ^=xor ":" Python ^=** "):" Xcas ";
  status += giac::angle_radian(contextptr)?"RAD ":"DEG ";
  status += ustl::string(session_filename);
  strcpy(xcas_status,status.c_str());
  DefineStatusMessage(xcas_status, 1, 0, 0);
  DisplayStatusArea();
}

#if 0
void delete_clipboard(){
  MCSDelVar2("@REV2","CLIP");
}

void copy_clipboard(const ustl::string & ans,bool status){
  CLIP_Store((unsigned char*)ans.c_str(), ans.size());
}

const char * paste_clipboard(){
  int start = 0;
  int cursor = 0;
  int ekey = KEY_CTRL_PASTE;
  EditMBStringCtrl2( (unsigned char*)text, textsize-1, &start, &cursor, &ekey, 1, 1*24-24, 1, 20 );
  Cursor_SetFlashOff();
  return text;
}

#else
void delete_clipboard(){}

bool clip_pasted=false;
  
ustl::string * clipboard(){
  static ustl::string * ptr=0;
  if (!ptr)
    ptr=new ustl::string;
  return ptr;
}

void copy_clipboard(const ustl::string & s,bool status){
  if (clip_pasted)
    *clipboard()=s;
  else
    *clipboard()+=s;
  clip_pasted=false;
  if (status){
    DefineStatusMessage((char*)(lang?"Selection copiee vers presse-papiers.":"Selection copied to clipboard"), 1, 0, 0);
    DisplayStatusArea();
  }
}

const char * paste_clipboard(){
  clip_pasted=true;
  return clipboard()->c_str();
}
#endif

int print_msg12(const char * msg1,const char * msg2,int textY=40){
  drawRectangle(0, textY+10, LCD_WIDTH_PX, 60, COLOR_WHITE);
  drawRectangle(3,textY+10,380,3, COLOR_BLACK);
  drawRectangle(3,textY+10,3,60, COLOR_BLACK);
  drawRectangle(380,textY+10,3,60, COLOR_BLACK);
  drawRectangle(3,textY+70,380,3, COLOR_BLACK);
  int textX=30;
  if (msg1)
    PrintMini(&textX,&textY,(unsigned char *)msg1,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); 
  textX=10;
  textY+=25;
  if (msg2)
    PrintMini(&textX,&textY,(unsigned char *)msg2,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); 
  Bdisp_PutDisp_DD();
  return textX;
}

void insert(string & s,int pos,const char * add){
  if (pos>s.size())
    pos=s.size();
  if (pos<0)
    pos=0;
  s=s.substr(0,pos)+add+s.substr(pos,s.size()-pos);
}

int inputline(const char * msg1,const char * msg2,ustl::string & s,bool numeric,int ypos=65){
  int X1=print_msg12(msg1,msg2,ypos-25);
  // s="";
  int pos=s.size(),beg=0;
  for (;;){
    int textX=X1,textY=ypos;
    drawRectangle(textX,textY+24,LCD_WIDTH_PX-textX-4,18,COLOR_WHITE);
    if (pos-beg>36)
      beg=pos-12;
    if (int(s.size())-beg<36)
      beg=giac::giacmax(0,int(s.size())-36);
    textX=X1;
    PrintMini(&textX,&textY,(unsigned char *)s.substr(beg,pos-beg).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    int cursorpos=textX;
    PrintMini(&textX,&textY,(unsigned char *)s.substr(pos,s.size()-pos).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    drawRectangle(cursorpos,textY+24,3,18,COLOR_BLACK); // cursor
    int key;
    GetKey(&key);
    if (!giac::freeze)
      set_xcas_status();    
    if (key==KEY_CTRL_F5){
      handle_f5();
      continue;
    }
    if (key==KEY_CTRL_EXE)
      return key;
    if (key==KEY_CHAR_IMGNRY)
      key='i';
    if (key==KEY_CHAR_MINUS || key==KEY_CHAR_PMINUS)
      key='-';
    if (key==KEY_CHAR_PLUS)
      key='+';
    if (key==KEY_CHAR_MULT)
      key='*';
    if (key==KEY_CHAR_DIV || key==KEY_CHAR_FRAC)
      key='/';
    if (key>=32 && key<128){
      if (!numeric || key=='-' || (key>='0' && key<='9'))
	s.insert(s.begin()+pos,char(key));
      ++pos;
      continue;
    }
    if (key==KEY_CTRL_XTT){
      insert(s,pos,xthetat?"t":"x");
      pos +=1;
      continue;
    }    
    if (key==KEY_CHAR_EXPN){
      insert(s,pos,"exp(");
      pos +=4;
      continue;
    }    
    if (key==KEY_CHAR_EXPN10){
      insert(s,pos,"10^");
      pos +=4;
      continue;
    }    
    if (key==KEY_CHAR_SIN){
      insert(s,pos,"sin(");
      pos +=4;
      continue;
    }
    if (key==KEY_CHAR_COS){
      insert(s,pos,"cos(");
      pos +=4;
      continue;
    }
    if (key==KEY_CHAR_TAN){
      insert(s,pos,"tan(");
      pos +=4;
      continue;
    }
    if (key==KEY_CHAR_ASIN){
      insert(s,pos,"asin(");
      pos +=5;
      continue;
    }
    if (key==KEY_CHAR_ACOS){
      insert(s,pos,"acos(");
      pos +=5;
      continue;
    }
    if (key==KEY_CHAR_EXP){
      insert(s,pos,"e");
      pos +=1;
      continue;
    }
    if (key==KEY_CHAR_PI){
      insert(s,pos,"pi");
      pos +=2;
      continue;
    }
    if (key==KEY_CHAR_ATAN){
      insert(s,pos,"atan(");
      pos +=5;
      continue;
    }
    if (key==KEY_CHAR_LN){
      insert(s,pos,"ln(");
      pos +=3;
      continue;
    }
    if (key==KEY_CHAR_LOG){
      insert(s,pos,"log10(");
      pos +=6;
      continue;
    }
    if (key==KEY_CHAR_VALR){
      insert(s,pos,"abs(");
      pos +=4;
      continue;
    }
    if (key==KEY_CHAR_THETA){
      insert(s,pos,"arg(");
      pos +=4;
      continue;
    }
    if (key==KEY_CHAR_RECIP){
      insert(s,pos,"^-1");
      pos +=3;
      continue;
    }
    if (key==KEY_CHAR_SQUARE){
      insert(s,pos,"^2");
      pos +=2;
      continue;
    }
    if (key==KEY_CHAR_CUBEROOT){
      insert(s,pos,"^(1/3)");
      pos +=6;
      continue;
    }
    if (key==KEY_CHAR_POWROOT){
      insert(s,pos,"^(1/");
      pos +=4;
      continue;
    }
    if (key==KEY_CTRL_DEL){
      if (pos){
	s.erase(s.begin()+pos-1);
	--pos;
      }
      continue;
    }
    if (key==KEY_CTRL_AC){
      if (s=="")
	return KEY_CTRL_EXIT;
      s="";
      pos=0;
      continue;
    }
    if (key==KEY_CTRL_EXIT)
      return key;
    if (key==KEY_CTRL_RIGHT){
      if (pos<s.size())
	++pos;
      continue;
    }
    if (key==KEY_SHIFT_RIGHT){
      pos=s.size();
      continue;
    }
    if (key==KEY_CTRL_LEFT){
      if (pos)
	--pos;
      continue;
    }
    if (key==KEY_SHIFT_LEFT){
      pos=0;
      continue;
    }
  }
}

int get_line_number(const char * msg1,const char * msg2){
  ustl::string s;
  int res=inputline(msg1,msg2,s,false);
  if (res==KEY_CTRL_EXIT)
    return 0;
  res=strtol(s.c_str(),0,10);
  return res;
}

void cleanup(ustl::string & s){
  for (size_t i=0;i<s.size();++i){
    if (s[i]=='\n')
      s[i]=' ';
  }
}

bool do_confirm(const char * s){
  return confirm(s,(lang?"F1: oui,    F6:annuler":"F1: yes,     F6: cancel"))==KEY_CTRL_F1;
}

bool confirm_overwrite(){
  return do_confirm(lang?"F1: oui,    F6:annuler":"F1: yes,     F6: cancel")==KEY_CTRL_F1;
}

void invalid_varname(){
  confirm(lang?"Nom de variable incorrect":"Invalid variable name", lang?"F1 ou F6: ok":"F1 or F6: ok");
}

void warn_python(int mode,bool autochange){
  if (mode==0)
    confirm(autochange?(lang?"Source en syntaxe Xcas detecte.":"Xcas syntax source code detected."):(lang?"Syntaxe Xcas.":"Xcas syntax."),"F1/F6: ok");
  if (mode==1)
    if (autochange)
      confirm(lang?"Source en syntaxe Python. Passage":"Python syntax source detected. Setting",lang?"en Python avec ^=**, F1/F6: ok":"Python mode with ^=**, F1/F6:ok");
    else
      confirm(lang?"Syntaxe Python avec ^==**, tapez":"Python syntax with ^==**, type",lang?"python_compat(2) pour xor. F1: ok":"python_compat(2) for xor. F1: ok");
  if (mode==2){
    confirm(lang?"Syntaxe Python avec ^==xor":"Python syntax with ^==xor",lang?"python_compat(1) pour **. F1: ok":"python_compat(1) for **. F1: ok");
  }
}

const char * input_matrix(bool list){
  static ustl::string * sptr=0;
  if (!sptr)
    sptr=new ustl::string;
  *sptr="";
  giac::gen v(giac::_VARS(0,contextptr));
  giac::vecteur w;
  if (v.type==giac::_VECT){
    for (size_t i=0;i<v._VECTptr->size();++i){
      giac::gen & tmp = (*v._VECTptr)[i];
      if (tmp.type==giac::_IDNT){
	giac::gen tmpe(eval(tmp,1,contextptr));
	if (list){
	  if (tmpe.type==giac::_VECT && !ckmatrix(tmpe))
	    w.push_back(tmp);
	}
	else {
	  if (ckmatrix(tmpe))
	    w.push_back(tmp);
	}
      }
    }
  }
  ustl::string msg;
  if (w.empty())
    msg=lang?"Creer nouveau":"Create new";
  else
    msg=((lang?"Creer nouveau ou editer ":"Create new or edit ")+(w.size()==1?w.front():giac::gen(w,giac::_SEQ__VECT)).print(contextptr));
  handle_f5();
  if (inputline(msg.c_str(),(lang?"Nom de variable:":"Variable name:"),*sptr,false) && !sptr->empty() && isalpha((*sptr)[0])){
    giac::gen g(*sptr,contextptr);
    giac::gen ge(eval(g,1,contextptr));
    if (g.type==giac::_IDNT){
      if (ge.type==giac::_VECT){
	ge=eqw(ge,true);
	ge=eval(ge,1,contextptr);
	if (ge.type==giac::_VECT)
	  sto(ge,g,contextptr);
	else
	  cout << "edited " << ge << endl;
	// *sptr += ":="+ge.print(contextptr)+":;";
	// cleanup(*sptr);
	return ""; // return sptr->c_str();
      }
      if (ge==g || confirm_overwrite()){
	*sptr="";
	if (inputline((lang?"Nombre de lignes":"Line number"),"",*sptr,true)){
	  int l=strtol(sptr->c_str(),0,10);
	  if (l>0 && l<256){
	    int c;
	    if (list)
	      c=0;
	    else {
	      ustl::string tmp(*sptr+(lang?" lignes.":" lines."));
	      *sptr="";
	      inputline(tmp.c_str(),lang?"Colonnes:":"Columns:",*sptr,true);
	      c=strtol(sptr->c_str(),0,10);
	    }
	    if (c==0){
	      ge=giac::vecteur(l);
	    }
	    else {
	      if (c>0 && l*c<256)
		ge=giac::_matrix(giac::makesequence(l,c),contextptr);
	    }
	    ge=eqw(ge,true);
	    ge=eval(ge,1,contextptr);
	    if (ge.type==giac::_VECT)
	      sto(ge,g,contextptr);
	    return "";
	  } // l<256
	}
      } // ge==g || overwrite confirmed
    } // g.type==_IDNT
    else {
      invalid_varname();
    }	
  } // isalpha
  return 0;
}

int confirm(const char * msg1,const char * msg2,bool acexit){
  print_msg12(msg1,msg2);
  int key=0;
  while (key!=KEY_CTRL_F1 && key!=KEY_CTRL_F6){
    GetKey(&key);
    if (key==KEY_CTRL_EXE)
      key=KEY_CTRL_F1;
    if (key==KEY_CTRL_AC || key==KEY_CTRL_EXIT){
      if (acexit) return -1;
      key=KEY_CTRL_F6;
    }
    set_xcas_status();
  }
  return key;
}  

#if 1
int get_filename(char * filename,const char * extension){
  handle_f5();
  ustl::string str;
  int res=inputline(lang?"EXIT ou chaine vide: annulation":"EXIT or empty string: cancel",lang?"Nom de fichier:":"Filename:",str,false);
  if (res==KEY_CTRL_EXIT || str.empty())
    return 0;
  strcpy(filename,"\\\\fls0\\");
  strcpy(filename+7,str.c_str());
  int s=strlen(filename);
  if (strcmp(filename+s-3,extension))
    strcpy(filename+s,extension);
  // if file already exists, warn, otherwise create
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0){
    save_script(filename,"");
    return 1;
    int size=1,BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    BCEres = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
    //cout << "create " << filename << " " << BCEres << endl;
    if (BCEres<0)
      return 0;
    return 1;
  }
  Bfile_CloseFile_OS(hFile);
  if (confirm(lang?"     Le fichier existe!":"     File exists!",lang?"F1: ecraser,           F6: annuler":"F1:overwrite,           F6: cancel")==KEY_CTRL_F1)
    return 1;
  return 0;
}
#else
int get_filename(char * filename){
  SetSetupSetting( (unsigned int)0x14, 0x88);	
  DisplayStatusArea();
  dConsolePut(lang?"\nNom du programme?\nPour annuler, laisser vide.\n:":"\nType a name for the script, or\nleave empty to cancel.\n:");
  char inputname[MAX_FILENAME_SIZE+1];
  inputname[0] = 0;
  gets(inputname,MAX_FILENAME_SIZE-50);
  puts(inputname);
  myconsoley += 5;
  if(!strlen(inputname)) {
    // user aborted
    puts(lang?"Annulation.":"Cancelled.");
    return 0;
  }
  strcpy(filename,"\\\\fls0\\");
  strcpy(filename+7,inputname);
  int s=strlen(filename);
  if (strcmp(filename+s-3,".py"))
    strcpy(filename+s,".py");
  puts(filename);
  // if file already exists, warn, otherwise create
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, filename, strlen(filename)+1); 
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0){
    int size,BCEres = Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    if (BCEres<0)
      return 0;
    return 1;
  }
  Bfile_CloseFile_OS(hFile);
  if (confirm(lang?"     Le fichier existe!":"     File exists!",lang?"F1: ecraser,           F6: annuler":"F1:overwrite,           F6: cancel")==KEY_CTRL_F1)
    return 1;
  return 0;
}
#endif

ustl::string get_searchitem(ustl::string & replace){
  replace="";
  ustl::string search;
  handle_f5();
  int res=inputline(lang?"EXIT ou chaine vide: annulation":"EXIT or empty string: cancel",lang?"Chercher:":"Search:",search,false);
  if (search.empty() || res==KEY_CTRL_EXIT)
    return "";
  replace="";
  ustl::string tmp=(lang?"EXIT: recherche seule de ":"EXIT: search only ")+search;
  handle_f5();
  res=inputline(tmp.c_str(),lang?"Remplacer par:":"Replace by:",replace,false);
  if (res==KEY_CTRL_EXIT)
    replace="";
  return search;
}

void initializeConsoleMemory(line_row* area) {
  line_buf = area;
}

void locate(int x, int y) {
  myconsolex = x-1;
  myconsoley = y-1;
}
int enable_bracket_coloring = 0;
int last_bracket_color = COLOR_BLUE;
int bracket_level = 0;

#if 0
void print(unsigned char* msg) {
  // stop at 0 or at 0x1e
  size_t s=strlen(msg),i;
  for (i=1;i<s;++i){
    if (msg[i]==0x1e){
      msg[i]=0;
      break;
    }
  }
  int linestart = line_count-visible_lines+myconsolescroll;
  if(linestart < 0) linestart = 0;
  if(myconsoley - linestart >= 0 && myconsoley - linestart < visible_lines)
    PrintMiniFix( myconsolex*12, (myconsoley - linestart)*17, msg, (visible_lines == 12 ? 0x40 : 0), COLOR_BLACK, COLOR_WHITE, (visible_lines == 12 ? 1 : 0));
  myconsolex = myconsolex + i;
  if (i<s) msg[i]=0x1e;
}
#else
void print(unsigned char* msg) {
  int linestart = line_count-visible_lines+myconsolescroll;
  if(linestart < 0) linestart = 0;
  if(myconsoley - linestart >= 0 && myconsoley - linestart < visible_lines) PrintMiniFix( myconsolex*12, (myconsoley - linestart)*17, msg, (visible_lines == 12 ? 0x40 : 0), COLOR_BLACK, COLOR_WHITE, (visible_lines == 12 ? 1 : 0));
  myconsolex = myconsolex + strlen((char*)msg);
}
#endif

char dGetKeyChar (uint key)
{
  if (key>=KEY_CHAR_A && key<=KEY_CHAR_Z)
    return key;
  if (key>=KEY_CHAR_LOWER_A && key<=KEY_CHAR_LOWER_Z)
    return key;
  if (key>=KEY_CHAR_0 && key<= KEY_CHAR_9)
    return key;
  if (key>=' ' && key<='~')
    return key;
  if (key==0xAB) // ! when typed on emulator
    return '!';
  switch(key)
  {
    case KEY_CHAR_PLUS:
    case KEY_CHAR_MINUS:
    case KEY_CHAR_MULT:
    case KEY_CHAR_DIV:
    case KEY_CHAR_POW:
      return key;
  }

  return 0;
}

void dConsoleCls ()
{
  line_index      = 0;
  line_x          = 0;
  line_start      = 0;
  line_count      = 0;
  myconsolescroll = 0;
  dConsoleRedraw();
}

void printCursor() {
  int x = myconsolex*12;
  int linestart = line_count-visible_lines+myconsolescroll;
  if(linestart < 0) linestart = 0;
  if(!(myconsoley - linestart >= 0 && myconsoley - linestart < visible_lines))
    return;
  int y = (myconsoley-linestart)*17+(visible_lines == 12 ? 0 : 24);
  // vertical cursor...
  drawLine(x, y+14, x, y, COLOR_BLACK);
  drawLine(x+1, y+14, x+1, y, COLOR_BLACK); 
}

void do_up_arrow(void);
void do_down_arrow(void);
extern int custom_key_to_handle;
extern int custom_key_to_handle_modifier;
int get_custom_key_handler_state();
//int get_set_session_setting(int value);
void create_data_folder();

// inserts into subject at position pos. assumes subject has enough space!
//append(s, (char*)"tan(", pos);
void append(char* subject, const char* insert, int pos, int inslen) {
  char buf[INPUTBUFLEN+5];
  strcpy(buf, subject+pos); // backup everything in the subject, after pos
  strcpy(subject+pos, insert); // copy string to insert into subject at desired position
  strcpy(subject+pos+inslen, buf); // restore backup at the end of the inserted string
}

void addStringToInput(char* dest, const char* src, int* pos, int max, int* refresh) {
  int srclen = strlen(src);
  if ((int)strlen(dest)+srclen-1>=max) return;
  append(dest, src, *pos, srclen);
  *pos+=srclen; *refresh = 1;
}

void do_refresh(char * s,int start,int pos,int x,int y,int width,int isscrolling){
  for (int i=x;i<=LINE_COL_MAX;++i) {
    locate(i,y); print((uchar*)" ");
  }
  last_bracket_color = COLOR_BLUE;
  bracket_level = 0;
  if (start==0) {
    enable_bracket_coloring = 1;
    locate(x,y);          print((uchar*)s);
    enable_bracket_coloring = 0;
    locate(x+pos,y); printCursor();// if (pos>=0){ locate(x+pos,y); printCursor(); }
  } else { // find current color for brackets
    for(int i = 0; i < start; ++i) {
      if(s[i] == '(') {
	bracket_level++;
	last_bracket_color = getNextColorInSequence(last_bracket_color);
      } else if(s[i] == ')' && bracket_level > 0) {
	last_bracket_color = getPreviousColorInSequence(last_bracket_color);
	bracket_level--;
      }
    }
    enable_bracket_coloring = 1;
    locate(x,y);          print((uchar*)s+start);
    enable_bracket_coloring = 0;
#if 1
    locate(pos-start+2,y);  printCursor(); //cursor
#else
    if (pos>=0){ //cursor 
      //locate(pos-start+1,y);
      locate(pos-start+2,y);
      printCursor();
    }
#endif
  }
  if(!isscrolling) {
    int linestart = line_count-visible_lines+myconsolescroll;
    if(linestart < 0) linestart = 0;
    int py = (y-linestart-1)*17;
    if(start) {
      int px = 0; //12;
      PrintMini(&px, &py, (unsigned char*)"\xe6\x9a", 0x02, 0xFFFFFFFF, 0, 0, COLOR_MAGENTA, COLOR_WHITE, 1, 0);
    }
    if((int)strlen(s+start)>width) {
      int px = 31*12;
      PrintMini(&px, &py, (unsigned char*)"\xe6\x9b", 0x02, 0xFFFFFFFF, 0, 0, COLOR_MAGENTA, COLOR_WHITE, 1, 0);
    }
  }
}

int handle_f5(){
  int keyflag = GetSetupSetting( (unsigned int)0x14);
  if (keyflag == 0x04 || keyflag == 0x08 || keyflag == 0x84 || keyflag == 0x88) {
    // ^only applies if some sort of alpha (not locked) is already on
    if (keyflag == 0x08 || keyflag == 0x88) { //if lowercase
      SetSetupSetting( (unsigned int)0x14, keyflag-0x04);
      DisplayStatusArea();
      return 1; //do not process the key, because otherwise we will leave alpha status
    } else {
      SetSetupSetting( (unsigned int)0x14, keyflag+0x04);
      DisplayStatusArea();
      return 1; //do not process the key, because otherwise we will leave alpha status
    }
  }
  if (keyflag==0) {
    SetSetupSetting( (unsigned int)0x14, 0x88);	
    DisplayStatusArea();
  }
  return 0;
}

giac::gen select_var(){
  giac::gen g(giac::_VARS(0,contextptr));
  if (g.type!=giac::_VECT || g._VECTptr->empty())
    return giac::undef;
  giac::vecteur & v=*g._VECTptr;
  MenuItem smallmenuitems[v.size()+2];
  vector<ustl::string> vs(v.size());
  int i=0;
  for (;i<v.size();++i){
    vs[i]=v[i].print(contextptr);
    smallmenuitems[i].text=(char *) vs[i].c_str();
  }
  smallmenuitems[i].text="purge(";
  smallmenuitems[i+1].text="assume(";
  Menu smallmenu;
  smallmenu.numitems=v.size()+1; 
  smallmenu.items=smallmenuitems;
  smallmenu.height=8;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char*)"Variables";
  //MsgBoxPush(5);
  int sres = doMenu(&smallmenu);
  //MsgBoxPop();
  if (sres!=MENU_RETURN_SELECTION)
    return giac::undef;
  if (smallmenu.selection==1+v.size())
    return giac::string2gen("purge(",false);
  if (smallmenu.selection==2+v.size())
    return giac::string2gen("assume(",false);
  return v[smallmenu.selection-1];
}

const char * keytostring(int key,int keyflag){
  const int textsize=1024;
  bool py=python_compat(contextptr);
  bool alph=keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
  static char text[textsize];
  switch (key){
  case KEY_CHAR_PLUS:
    return "+";
  case KEY_CHAR_MINUS:
    return "-";
  case KEY_CHAR_PMINUS:
    return "_";
  case KEY_CHAR_MULT:
    return "*";
  case KEY_CHAR_FRAC:
    return py?"\\":"solve(";
  case KEY_CHAR_DIV: 
    return "/";
  case KEY_CHAR_POW:
    return "^";
  case KEY_CHAR_ROOT:
    return "sqrt(";
  case KEY_CHAR_SQUARE:
    return py?"**2":"^2";
  case KEY_CHAR_CUBEROOT:
    return py?"**(1/3)":"^(1/3)";
  case KEY_CHAR_POWROOT:
    return py?"**(1/":"^(1/";
  case KEY_CHAR_RECIP:
    return py?"**-1":"^-1";
  case KEY_CHAR_THETA:
    return "arg(";
  case KEY_CHAR_VALR:
    return "abs(";
  case KEY_CHAR_ANGLE:
    return "polar_complex(";
  case KEY_CTRL_XTT:
    return xthetat?"t":"x";
  case KEY_CHAR_LN:
    return "ln(";
  case KEY_CHAR_LOG:
    return "log10(";
  case KEY_CHAR_EXPN10:
    return "10^";
  case KEY_CHAR_EXPN:
    return "exp(";
  case KEY_CHAR_SIN:
    return "sin(";
  case KEY_CHAR_COS:
    return "cos(";
  case KEY_CHAR_TAN:
    return "tan(";
  case KEY_CHAR_ASIN:
    return "asin(";
  case KEY_CHAR_ACOS:
    return "acos(";
  case KEY_CHAR_ATAN:
    return "atan(";
  case KEY_CTRL_MIXEDFRAC:
    return "limit(";
  case KEY_CTRL_FRACCNVRT:
    return "exact(";
  case KEY_CTRL_FORMAT:
    return "purge(";
  case KEY_CTRL_FD:
    return "approx(";
  case KEY_CHAR_STORE:
    // if (keyflag==1) return "inf";
    return "=>";
  case KEY_CHAR_IMGNRY:
    return "i";
  case KEY_CHAR_PI:
    return "pi";
  case KEY_CTRL_VARS: {
    giac::gen var=select_var();
    if (!giac::is_undef(var)){
      strcpy(text,(var.type==giac::_STRNG?*var._STRNGptr:var.print(contextptr)).c_str());
      return text;
    }
    return "VARS()";
  }
  case KEY_CHAR_EXP:
    return "e";
  case KEY_CHAR_ANS:
    return "ans()";
  case KEY_CTRL_INS:
    return "inf";
  case KEY_CHAR_COMMA:
    // if (keyflag==1) return "solve(";
    return ",";
  case KEY_CTRL_F1:
    if (alph) return ";";
    if (keyflag==1) return "!";
    return py?":":":=";
  case KEY_CTRL_F2:
    return (alph)?"#":(keyflag==1?"<":">");
  case KEY_CTRL_F3:
    return (alph)?(py?"\\":"sum("):(keyflag==1?(py?"%":"integrate("):((!py && xthetat)?"diff(":"'"));
  case KEY_CHAR_MAT:{
    // const char * ptr=input_matrix(false); if (ptr) return ptr;
    if (showCatalog(text,17)) return text;
    return "";
  }
  case KEY_CHAR_LIST: {
    //const char * ptr=input_matrix(true); if (ptr) return ptr;
    if (showCatalog(text,16)) return text;
    return "";
  }
  case KEY_CTRL_PRGM:
      // open functions catalog, prgm submenu
    if(showCatalog(text,18))
      return text;
    return "";
  case KEY_CTRL_CATALOG:
    if(showCatalog(text,1)) 
      return text;
    return "";
  case KEY_CTRL_F4:
    if(showCatalog(text,0)) 
      return text;
    return "";
  case KEY_SHIFT_OPTN:
    if(showCatalog(text,10))
      return text;
    return "";
  case KEY_CTRL_OPTN:
    if(showCatalog(text,15))
      return text;
    return "";
  case KEY_CTRL_QUIT: 
    if(showCatalog(text,20))
      return text;
    return "";
  case KEY_CTRL_SETUP:
    if(showCatalog(text,7))
      return text;
    return "";
  case KEY_CTRL_PASTE:
    return paste_clipboard();
  }
  return 0;
}

void erase_script();

int dGetLine (char * s,int max, int isRecording, int ml) {
  int pos = strlen(s);
  int start = 0;
  int refresh = 1;
  int x,y,l,width;
  int key;
  char c;
  l = strlen(line_buf[line_index]);

  if (l>=LINE_COL_MAX) {
    dConsolePut("\n");
    l = 0;
  } else dConsoleRedraw();

  x = l + 1;
  y = line_count;
  width = LINE_COL_MAX - l;
  int firstLoopRun = 1;
  int isscrolling = 0;
  while (1) {
    // insure start<=pos<=start+width-1
    if(pos-1<start) {
      do start--; while (pos-1<start);
      if (start<0) start = 0;
    } else if(pos>start+width-1) {
      do start++; while (pos>start+width-1);
    }
    if(isscrolling) {
      DefineStatusMessage((char*)"\xE5\xEA/\xE5\xEB to scroll, other key to go back", 1, 0, 0);
      DisplayStatusArea();
    }
    if (refresh){
      do_refresh(s,start,pos,x,y,width,isscrolling);
      refresh = 0;
    }
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    drawRectangle(LCD_WIDTH_PX-6, 24, 5, LCD_HEIGHT_PX-30, COLOR_WHITE);
    if(isscrolling) {
      int starty = (line_count == 0 ? 0 : ((line_count+myconsolescroll-(line_count < visible_lines ? line_count : visible_lines))*162)/(line_count < visible_lines ? line_count : line_count-visible_lines));
      drawRectangle(LCD_WIDTH_PX-6, 24+starty, 5, 8, COLOR_BLACK);
    }
    GetKey(&key);
    set_xcas_status();
    //addStringToInput(s, giac::print_INT_(key).c_str(), &pos, max, &refresh);
    if(isscrolling && !(key==KEY_CTRL_PAGEUP || key==KEY_CTRL_PAGEDOWN || key==KEY_CTRL_UP || key==KEY_CTRL_DOWN)) {
      isscrolling = 0;
      myconsolescroll = 0;
      DefineStatusMessage((char*)"", 1, 0, 0);
      dConsoleRedraw();
      refresh = 1;
    }
    if(key == KEY_CTRL_F5) {
      handle_f5();
      continue;
    }
    if (key==KEY_CHAR_FRAC){
      giac::gen g;
      if (strlen(s)!=0){
	g=giac::gen(s,contextptr);
	int lineerr=giac::first_error_line(contextptr);      
	if (lineerr){
	  for (int i=0;i<sizeof(xcas_status);++i)
	    xcas_status[i]=0;
	  sprintf(xcas_status,lang?"Erreur a %s":"Error at %s",giac::error_token_name(contextptr).c_str());
	  confirm(xcas_status,"Press F1 or F6");
	  return 7;
	}
      }
      g=eqw(g,false);
      if (is_undef(g))
	return 7;
      string ss(g.print(contextptr));
      if (ss.size()>max)
	ss=ss.substr(0,max);
      strcpy(s,ss.c_str());
      pos=strlen(s);
      return 7;
    }
    if ( (key==KEY_CHAR_STORE || key==KEY_CHAR_PLUS || key==KEY_CHAR_MINUS || key==KEY_CHAR_MULT || key==KEY_CHAR_DIV || key==KEY_CHAR_POW || key==KEY_CHAR_SQUARE)
	 && *s==0 && firstLoopRun)
      addStringToInput(s, "ans()", &pos, max, &refresh); //start of line, append "last" as we're going to do a calculation on the previous value
    const char * adds=keytostring(key,keyflag);
    if (adds){
      bool islist=strcmp(adds,"list ")==0,
	ismat=strcmp(adds,"matrix ")==0;
      if (islist)
	input_matrix(true);
      if (ismat)
	input_matrix(false);
      if (!islist && !ismat)
	addStringToInput(s,adds,&pos,max,&refresh);
      refresh=1;
      dConsoleRedraw();
      continue;
    }
    if (key==KEY_CTRL_EXIT && edptr){
      return -3;
      doTextArea(edptr);
      //cout << myconsoley << " " << line_index << " " << line_start << endl;
      myconsoley=line_index;
      cout << endl; dConsolePutChar('\x1e');
      refresh=1;
      dConsoleRedraw();
      continue;
    }
    if (key==KEY_CTRL_CLIP) {
      // allow for copying last result or current write buffer
      MsgBoxPush(4);
      MenuItem smallmenuitems[5];
      smallmenuitems[0].text = (char*)"Last result";
      smallmenuitems[1].text = (char*)"Current command";
      
      Menu smallmenu;
      smallmenu.items=smallmenuitems;
      smallmenu.numitems=2;
      smallmenu.width=17;
      smallmenu.height=4;
      smallmenu.startX=3;
      smallmenu.startY=2;
      smallmenu.scrollbar=0;
      smallmenu.title = (char*)"Clip on:";
      int sres = doMenu(&smallmenu);
      MsgBoxPop();
      
      if(sres == MENU_RETURN_SELECTION) {
	delete_clipboard();
        if(smallmenu.selection == 1) {
	  ustl::string ans=last_ans();
          if(!ans.empty()) {
	    copy_clipboard(ans,false);
	    DefineStatusMessage((char*)(lang?"Reponse copiee vers presse-papiers.":"Last result copied to clipboard"), 1, 0, 0);
	    DisplayStatusArea();
          } else AUX_DisplayErrorMessage( 0x15 );
        } else if(smallmenu.selection == 2) {
	  copy_clipboard(s,true);
        }
      }
      refresh = 1;
      dConsoleRedraw();
    } else if(key==KEY_CTRL_F6) {
      Menu smallmenu;
      smallmenu.numitems=16;
      MenuItem smallmenuitems[smallmenu.numitems];
      
      smallmenu.items=smallmenuitems;
      smallmenu.height=8;
      smallmenu.scrollbar=1;
      smallmenu.scrollout=1;
      //smallmenu.title = "KhiCAS";
      smallmenuitems[0].text = (char *) (lang?"Enregistrer session":"Save session ");
      smallmenuitems[1].text = (char *) (lang?"Enregistrer sous":"Save session as");
      smallmenuitems[2].text = (char*) (lang?"Charger session":"Load session");
      smallmenuitems[3].text = (char*)(lang?"Nouvelle session":"New session");
      smallmenuitems[4].text = (char*)lang?"Nouveau script":"New script";
      smallmenuitems[5].text = (char*)lang?"Editer script":"Edit script";
      smallmenuitems[6].text = (char*)lang?"Executer script":"Run script";
      smallmenuitems[7].text = (char*)lang?"Editer matrice":"Matrix editor";
      smallmenuitems[8].text = (char*)lang?"Effacer":"Clear";
      smallmenuitems[9].text = (char*)lang?"Effacer script":"Clear script";
      smallmenuitems[10].type = MENUITEM_CHECKBOX;
      smallmenuitems[10].text = (char*)"X,Theta,T=t";
      smallmenuitems[11].type = MENUITEM_CHECKBOX;
      smallmenuitems[11].text = (char*)"Python";
      smallmenuitems[12].type = MENUITEM_CHECKBOX;
      smallmenuitems[12].text = (char*)"Radians";
      smallmenuitems[13].type = MENUITEM_CHECKBOX;
      smallmenuitems[13].text = (char*)lang?"Sauvegarde auto.":"Auto save";
      smallmenuitems[14].text = (char *)aide_khicas_string;
      smallmenuitems[15].text = "A propos";
      // smallmenuitems[2].text = (char*)(isRecording ? "Stop Recording" : "Record Script");
      while(1) {
	smallmenuitems[10].value = xthetat;
	smallmenuitems[11].value = giac::python_compat(contextptr);
	smallmenuitems[12].value = giac::angle_radian(contextptr);
	smallmenuitems[13].value = get_set_session_setting(-1);
        MsgBoxPush(5);
        int sres = doMenu(&smallmenu);
        MsgBoxPop();
        if(sres == MENU_RETURN_SELECTION) {
	  const char * ptr=0;
	  if (smallmenu.selection==1){
	    if (strcmp(session_filename,"session")==0)
	      smallmenu.selection=2;
	    else {
	      save(session_filename);
	      dConsoleRedraw();set_xcas_status();
	      break;
	    }
	  }
	  if (smallmenu.selection==2){
	    char buf[270];
	    if (get_filename(buf,".xw")){
	      save(buf);
	      string fname(remove_path(giac::remove_extension(buf)));
	      strcpy(session_filename,fname.c_str());
	      if (edptr)
		edptr->filename="\\\\fls0\\"+fname+".py";
	    }
	    dConsoleRedraw();set_xcas_status();
	    break;
	  }
	  if (smallmenu.selection==3){
	    char filename[MAX_FILENAME_SIZE+1];
	    if (fileBrowser(filename, (char*)"*.xw", (char *)"Sessions")){
	      if (strcmp(session_filename,"session")==0 || confirm(lang?"Session courante perdue?":"Current session will be lost",lang?"F1: annul, F6: ok":"F1: cancel, F6: ok")==KEY_CTRL_F6){
		giac::_restart(giac::gen(giac::vecteur(0),giac::_SEQ__VECT),contextptr);
		restore_session(filename);
		strcpy(session_filename,remove_path(giac::remove_extension(filename)).c_str());
		// reload_edptr(session_filename,edptr);
	      }     
	    }
	    dConsoleRedraw();set_xcas_status();
	    break;
	  }
          if (smallmenu.selection==4) {
	    char filename[MAX_FILENAME_SIZE+1];
	    if (get_filename(filename,".xw")){
	      if (strcmp(session_filename,"session")==0 || confirm(lang?"Session courante perdue?":"Current session will be lost",lang?"F1: annul, F6: ok":"F1: cancel, F6: ok")==KEY_CTRL_F6){
		dConsoleCls();
		pos=0; myconsolex=1; myconsoley=0; myconsolescroll=0; x=2;y=1;
		dConsolePutChar('\x1e');
		refresh=1;
		dConsoleRedraw();
		giac::_restart(giac::gen(giac::vecteur(0),giac::_SEQ__VECT),contextptr);
		ustl::string s(remove_path(giac::remove_extension(filename)));
		strcpy(session_filename,s.c_str());
		reload_edptr(session_filename,edptr);
	      }
	    }  
	    dConsoleRedraw();set_xcas_status();
            break;
          }
          if(smallmenu.selection == 5) {
	    char filename[MAX_FILENAME_SIZE+1];
	    drawRectangle(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24, COLOR_WHITE);
	    if (get_filename(filename,"*.py"))
	      edit_script(filename);
	    // y += 5;
	    //start=0;
	    refresh=1;
	    //dConsolePutChar('\x1e');
	    //dConsolePut(s);
	    dConsoleRedraw();
            break;
          }
	  if (smallmenu.selection == 6) {
            return 3;
          }
	  if (smallmenu.selection == 7) {
            //if(ml) return 6;
            //strcpy(s, "record");
            //return 1;
	    return 2;
          }
          if(smallmenu.selection == 8){
	    drawRectangle(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24, COLOR_WHITE);
	    if (ptr=input_matrix(false)) {
	      addStringToInput(s,ptr,&pos,max,&refresh);
	      refresh=1;
	      dConsoleRedraw();
	      break;
	    }
	  }
	  if (smallmenu.selection == 9) {
	    chk_restart();
	    dConsoleCls();
	    pos=0; myconsolex=1; myconsoley=0; myconsolescroll=0; x=2;y=1;
	    dConsolePutChar('\x1e');
	    refresh=1;
	    dConsoleRedraw();
	    break;
	    //addStringToInput(s,"/* DEL EXE to */ restart?",&pos,max,&refresh);
          }
	  if (smallmenu.selection==10){
	    erase_script();
	    return 7;
	  }
	  if (smallmenu.selection >= 11 && smallmenu.selection <= 13) {
	    if (smallmenu.selection == 13)
	      giac::angle_radian(!giac::angle_radian(contextptr),contextptr);
	    else {
	      if (smallmenu.selection == 12){
		giac::python_compat(!giac::python_compat(contextptr),contextptr);
		warn_python(giac::python_compat(contextptr));
	      }
	      else xthetat=1-xthetat;
	    }
	    return 7; // return will update status area
	    break; // continue; 
	  }
	  if (smallmenu.selection == 14) {
	    get_set_session_setting(!get_set_session_setting(-1));
	    continue;
          }
	  if (smallmenu.selection >= 15) {
	    textArea text;
	    text.editable=false;
	    text.clipline=-1;
	    text.title = smallmenuitems[smallmenu.selection-1].text;
	    add(&text,smallmenu.selection==15?shortcuts_string:apropos_string);
	    doTextArea(&text);
	    continue;
          }
        }
        refresh = 1;
        dConsoleRedraw();
        break;
      }
    } else if (key==KEY_CTRL_UP) {
      if(isscrolling) {
        myconsolescroll--;
        if(line_count-visible_lines+myconsolescroll < 0) myconsolescroll++;
        dConsoleRedraw();
      } else {
        // go up in command history
        do_up_arrow();
        pos=strlen(s);
        start = 0; // force recalculation
      }
      refresh = 1;
    } else if (key==KEY_CTRL_DOWN) {
      // go down in command history
      if(isscrolling) {
        myconsolescroll++;
        if(myconsolescroll>0) myconsolescroll = 0;
        dConsoleRedraw();
      } else {
        do_down_arrow();
        pos=strlen(s);
        start = 0; // force recalculation
      }
      refresh = 1;
    } else if (key==KEY_CTRL_PAGEUP || key==KEY_CTRL_PAGEDOWN) {
      isscrolling = 1;
    } else if (key==KEY_CTRL_LEFT) {
      // move cursor left
      if(pos<=0) pos=strlen(s); //cycle
      else pos--;
      refresh = 1;
    } else if (key==KEY_CTRL_RIGHT) {
      // move cursor right
      if(pos>=(int)strlen(s)) pos=0; //cycle
      else pos++;
      refresh = 1;
    } else if ((c=dGetKeyChar(key))!=0) {
      char ns[2];
      ns[0] = c; ns[1]='\0';
      addStringToInput(s, ns, &pos, max, &refresh);
    }
    else if (key==KEY_CTRL_DEL) {
      if (pos<=0) continue;
      pos--;
      int i = pos;
      do {
              s[i] = s[i+1];
              i++;
      } while (s[i] != '\0');
      refresh = 1;
    }
    else if (key==KEY_CTRL_AC) {
      if (*s==0){
	if (confirm(lang?"Effacer l'historique?":"Clear history?",lang?"F1: oui,     F6: annuler":"F1: yes,     F6: cancel")==KEY_CTRL_F1){
	  dConsoleCls();
	  pos=0; myconsolex=1; myconsoley=0; myconsolescroll=0; x=2;y=1;
	  dConsolePutChar('\x1e');
	  refresh=1;
	}
	dConsoleRedraw();
      }
      *s = 0;
      pos = 0;
      refresh = 1;
    }
    else if (key==KEY_CTRL_EXE) return 1;
    else if (key==KEY_CHAR_CR){
#if 0
      addStringToInput(s, "\x1e", &pos, max, &refresh);
      s[pos-1]=0;
      do_refresh(s,start,-1,x,y,width,isscrolling); // no cursor
      s[pos-1]=0x1e;
      puts(s+start);
      dConsoleRedraw();
      start=pos+1;
      x=1; y=line_count;
#else
      addStringToInput(s, "\x1e", &pos, max, &refresh);
      return 5;
#endif
    }
    else if (key!=KEY_CTRL_SHIFT && key!=KEY_CTRL_ALPHA && get_custom_key_handler_state()==1) {
      custom_key_to_handle = key; custom_key_to_handle_modifier = keyflag; return 3;
    }
    firstLoopRun = 0;
  }
  return 0;
}

int get_custom_fkey_label(int key);
const int jaune=0xFDE0; // COLOR_BROWN
void draw_menu(int editor){
  drawFkeyLabels(get_custom_fkey_label(0), get_custom_fkey_label(1), get_custom_fkey_label(2), get_custom_fkey_label(3), 0x0307, get_custom_fkey_label(5));
  int fkeysup=195,fkeyw=LCD_WIDTH_PX/6;
  bool py=python_compat(contextptr);
#if 1
  int x,y=LCD_HEIGHT_PX-STATUS_AREA_PX-17;
  if (editor==2){
    x=0;
    PrintMini(&x,&y,(unsigned char *)"Edit",0,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
    x += 3;
    Bdisp_MMPrint(x,y-2,"+",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    x += 11; 
    Bdisp_MMPrint(x,y-2,"-",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
    x += 9;
    PrintMini(&x,&y,(unsigned char *)"simp",0,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
    x += 2;
    Bdisp_MMPrint(x,y-2,"*",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    // PrintMini(&x,&y,(unsigned char *)"<",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    x += 9;
    Bdisp_MMPrint(x,y-2,"+",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
    // PrintMini(&x,&y,(unsigned char *)"#",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
    x += 14;
  }
  else {
    x=fkeyw/2-20;
    PrintMini(&x,&y,(unsigned char *)(py?": ":":="),0,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
    x += 6;
    Bdisp_MMPrint(x,y-2,"!",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    x += 18; 
    //PrintMini(&x,&y,(unsigned char *)":=",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    Bdisp_MMPrint(x,y-2,";",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
  // PrintMini(&x,&y,(unsigned char *)";",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
    x += fkeyw-36;
    PrintMini(&x,&y,(unsigned char *)">",0,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
    x += 6;
    Bdisp_MMPrint(x,y-2,"<",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    // PrintMini(&x,&y,(unsigned char *)"<",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    x += 12;
    Bdisp_MMPrint(x,y-2,"#",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
    // PrintMini(&x,&y,(unsigned char *)"#",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
    x += fkeyw-36;
  }
  PrintMini(&x,&y,(unsigned char *)"'",0,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
  x += 2;
  // PrintMiniMini( &x, &y, (unsigned char *)"int", 0,6, 0 ); // yellow
  Bdisp_MMPrint(x,y-2,(py && editor<2)?" % ":"int",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
  x += 20;
  PrintMiniMini( &x, &y, (unsigned char*)((py && editor<2)?"\\":"sum"), 0,4, 0 ); // red
  // mPrintXY(2,8,":",TEXT_MODE_TRANSPARENT_BACKGROUND,TEXT_COLOR_BLACK);
  // mPrintXY(6,8,">",TEXT_MODE_TRANSPARENT_BACKGROUND,TEXT_COLOR_BLACK);
  // mPrintXY(10,8,"'",TEXT_MODE_TRANSPARENT_BACKGROUND,TEXT_COLOR_BLACK);
#else
  // :
  drawRectangle(fkeyw/2-10,LCD_HEIGHT_PX-16, 3, 3, TEXT_COLOR_BLACK);
  drawRectangle(fkeyw/2-10,LCD_HEIGHT_PX-8, 3, 3, TEXT_COLOR_BLACK);
  // >
  drawLine(fkeyw+fkeyw/2,LCD_HEIGHT_PX-19,fkeyw+fkeyw/2+7,LCD_HEIGHT_PX-12,TEXT_COLOR_BLACK);
  drawLine(fkeyw+fkeyw/2,LCD_HEIGHT_PX-18,fkeyw+fkeyw/2+6,LCD_HEIGHT_PX-12,TEXT_COLOR_BLACK);
  drawLine(fkeyw+fkeyw/2,LCD_HEIGHT_PX-5,fkeyw+fkeyw/2+7,LCD_HEIGHT_PX-12,TEXT_COLOR_BLACK);
  drawLine(fkeyw+fkeyw/2,LCD_HEIGHT_PX-6,fkeyw+fkeyw/2+6,LCD_HEIGHT_PX-12,TEXT_COLOR_BLACK);
  // '
  drawLine(2*fkeyw+fkeyw/2,LCD_HEIGHT_PX-12,2*fkeyw+fkeyw/2+6,LCD_HEIGHT_PX-18,TEXT_COLOR_BLACK);
  drawLine(2*fkeyw+fkeyw/2,LCD_HEIGHT_PX-11,2*fkeyw+fkeyw/2+6,LCD_HEIGHT_PX-17,TEXT_COLOR_BLACK);
#endif
  // drawLine(1,195,319,195,TEXT_COLOR_BLACK);
  drawLine(1,fkeysup,LCD_WIDTH_PX-1,fkeysup,TEXT_COLOR_BLACK);
  drawLine(1,LCD_HEIGHT_PX-1,LCD_WIDTH_PX-1,LCD_HEIGHT_PX-1,TEXT_COLOR_BLACK);
  drawLine(1,LCD_HEIGHT_PX-1,1,fkeysup,TEXT_COLOR_BLACK);
  // sep
  drawLine(fkeyw,LCD_HEIGHT_PX-1,fkeyw,fkeysup,TEXT_COLOR_BLACK);
  // sep
  drawLine(2*fkeyw,LCD_HEIGHT_PX-1,2*fkeyw,fkeysup,TEXT_COLOR_BLACK);
  drawLine(3*fkeyw,LCD_HEIGHT_PX-1,3*fkeyw,fkeysup,TEXT_COLOR_BLACK);
  // sep
  drawLine(LCD_WIDTH_PX-1,LCD_HEIGHT_PX-1,LCD_WIDTH_PX-1,fkeysup,TEXT_COLOR_BLACK);
#if 1
  Bdisp_MMPrint(3*fkeyw,LCD_HEIGHT_PX-STATUS_AREA_PX-18," CATALOG",0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
  if (editor==2){
    drawRectangle(5*fkeyw,LCD_HEIGHT_PX-18, fkeyw, 16, TEXT_COLOR_WHITE);
    x=5*fkeyw-2;
    y -= 2;
    PrintMini( &x, &y, (unsigned char *)"eval", 0,0xffffffff,0,0,COLOR_BLACK,COLOR_WHITE,1,0);
    Bdisp_MMPrint(x,y+1,"~",0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
    x += 10; 
    Bdisp_MMPrint(x,y+1,"><",0,0xffffffff,0,0,COLOR_RED,COLOR_WHITE,1,0);
  }
  else {
    drawRectangle(5*fkeyw,LCD_HEIGHT_PX-18, fkeyw, 16, TEXT_COLOR_BLACK);
    Bdisp_MMPrint(5*fkeyw+2,LCD_HEIGHT_PX-STATUS_AREA_PX-18,editor==2?" eval":(lang?"Fich,Cfg":"File,Cfg"),0,0xffffffff,0,0,COLOR_WHITE,COLOR_BLACK,1,0);
  }
#else
  mPrintXY(12,8,"cat",TEXT_MODE_INVERT,TEXT_COLOR_BLACK);
  mPrintXY(19,8,"men",TEXT_MODE_INVERT,TEXT_COLOR_BLACK);
#endif
}
void dConsoleRedraw() {
  draw_menu(0);
  // ^ user-defined, user-defined, user-defined, user-defined,  A<>a, user-defined
  drawRectangle(0, 0, LCD_WIDTH_PX, 24+17*visible_lines+1, COLOR_WHITE);
  for(int i=1,j=line_start; i<=line_count; ++i) {
    locate(1,i); print((uchar*)line_buf[j]);
    if (++j>=LINE_ROW_MAX) j = 0;
  }
  if(visible_lines != 12) DisplayStatusArea();
}
void dConsolePutChar (char c)
{
  if (!dconsole_mode)
    return;
  if(outputRedirectBuffer != NULL && remainingBytesInRedirect) {
    *outputRedirectBuffer = c;
    outputRedirectBuffer++;
    *outputRedirectBuffer = '\0';
    remainingBytesInRedirect--;
    return; // mute real console
  }
  if (line_count == 0) line_count = 1;
  if (c != '\n') line_buf[line_index][line_x++] = c;
  else
  {
    console_changed=1;
    line_buf[line_index][line_x] = '\0';
    line_x = LINE_COL_MAX;
  }
  if (line_x>=LINE_COL_MAX)
  {
    line_buf[line_index][line_x] = '\0';

    line_x = 0;
    if (line_count<LINE_ROW_MAX) ++line_count;
    else {
      line_start++;
      if (line_start>=LINE_ROW_MAX) line_start = 0;
    }
    line_index++;
    if (line_index>=LINE_ROW_MAX) line_index = 0;
  }
  line_buf[line_index][line_x] = '\0';
}

void dConsolePut(const char * str)
{
  if (!dconsole_mode)
    return;
  if (line_count == 0) line_count = 1;
  for (;*str;++str) dConsolePutChar(*str);
  line_buf[line_index][line_x] = '\0';
}

void dPuts(const char * s)
{
  dConsolePut(s);
  dConsolePutChar('\n');
}

/*int dPrintf (const char * format,...)
{
  char    buf[256];
  int             length;
  va_list arg_list;

  va_start(arg_list,format);
  length = vsprintf(buf,format,arg_list);
  va_end(arg_list);

  dConsolePut (buf);
  
  return length;
}*/

#if 1
void save_console_state_smem(const char * filename){
  // cout << "line count " << line_count << endl; return;
  console_changed=0;
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1);
  string state(khicas_state());
  int statesize=state.size();
  string script;
  if (edptr)
    script=merge_area(edptr->elements);
  int scriptsize=script.size();
  // save format: line_size (2), start_col(2), line_type (1), readonly (1), line
  int size=2*sizeof(int)+statesize+scriptsize;
  for (int i=1,j=line_start; i<=line_count; ++i) {
    size += 2*sizeof(short)+2*sizeof(char)+strlen(line_buf[j]);
    if (++j>=LINE_ROW_MAX) j = 0;
  }
  // cout << script << endl << "script size " << scriptsize << " size " << size; return;
  // there's no need to delete and create again, because there's no problem
  // if there's junk at the end of the file.
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0) {
    // could not open file. file might not exist yet, or the data folder might not exist at all.
    // try creating both and try opening again
    create_data_folder();
    Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE,(int *)&size);
    hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
    if(hFile < 0) return; // if it still fails, there's nothing we can do
  }
  // save variables and modes
  Bfile_WriteFile_OS(hFile, &statesize, sizeof(statesize));
  Bfile_WriteFile_OS(hFile, state.c_str(), statesize);
  // save script
  Bfile_WriteFile_OS(hFile, &scriptsize, sizeof(scriptsize));
  Bfile_WriteFile_OS(hFile, script.c_str(), scriptsize);
  // save console state
  for(int i=1,j=line_start; i<=line_count; ++i) {
    unsigned short l=strlen(line_buf[j]);
    Bfile_WriteFile_OS(hFile, &l, sizeof(l));
    unsigned short s=0;
    Bfile_WriteFile_OS(hFile, &s, sizeof(s));
    unsigned char c=0;
    Bfile_WriteFile_OS(hFile, &c, sizeof(c));
    c=0;
    Bfile_WriteFile_OS(hFile, &c, sizeof(c));
    Bfile_WriteFile_OS(hFile, line_buf[j], l);
    if (++j>=LINE_ROW_MAX) j = 0;
  }
  char buf[2]={0,0};
  Bfile_WriteFile_OS(hFile, buf, sizeof(buf));
  Bfile_CloseFile_OS(hFile);
}

bool load_console_state_smem(const char * filename){
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1);
  int hf = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if (hf < 0) return false; // nothing to load
  // int Bfile_ReadFile(int HANDLE,void *buf,int size,int readpos);
  // read variables and modes
  int L=0;
  if (Bfile_ReadFile_OS(hf,&L,sizeof(L),-1)!=sizeof(L) || L==0){
    Bfile_CloseFile_OS(hf);
    return false;
  }  
  char BUF[L+1];
  if (Bfile_ReadFile_OS(hf,BUF,L,-1)!=L){
    Bfile_CloseFile_OS(hf);
    return false;
  }
  BUF[L]=0;
  giac::gen g,ge;
  dconsole_mode=0;
  do_run((char*)BUF,g,ge);
  dconsole_mode=1;
  // read script
  if (Bfile_ReadFile_OS(hf,&L,sizeof(L),-1)!=sizeof(L)){
    Bfile_CloseFile_OS(hf);
    return false;
  }
  if (L>0){
    char bufscript[L+1];
    if (Bfile_ReadFile_OS(hf,bufscript,L,-1)!=L){
      Bfile_CloseFile_OS(hf);
      return false;
    }
    bufscript[L]=0;
    if (edptr==0)
      edptr=new textArea;
    if (edptr){
      edptr->elements.clear();
      edptr->clipline=-1;
      edptr->filename="\\\\fls0\\"+remove_path(giac::remove_extension(filename))+".py";
      //cout << "script " << edptr->filename << endl;
      edptr->editable=true;
      edptr->changed=false;
      edptr->python=python_compat(contextptr);
      edptr->elements.clear();
      edptr->y=0;
      add(edptr,bufscript);
      edptr->line=0;
      //edptr->line=edptr->elements.size()-1;
      edptr->pos=0;
    }    
  }
  // clear console and restore
  dConsoleCls();
  myconsolex=1; myconsoley=0; myconsolescroll=0; 
  for (int pos=0;;++pos){
    unsigned short int l,curs;
    unsigned char type,readonly;
    if (Bfile_ReadFile_OS(hf,&l,sizeof(l),-1)!=sizeof(l) || l==0) break;
    if (Bfile_ReadFile_OS(hf,&curs,sizeof(curs),-1)!=sizeof(curs)) break;
    if (Bfile_ReadFile_OS(hf,&type,sizeof(type),-1)!=sizeof(type)) break;
    if (Bfile_ReadFile_OS(hf,&readonly,sizeof(readonly),-1)!=sizeof(readonly)) break;
    char buf[l+1];
    buf[l]=0;
    if (Bfile_ReadFile_OS(hf,buf,l,-1)!=l) break;
    // ok line ready in buf
    update_cmd_history(buf);
    puts(buf);
  }
  Bfile_CloseFile_OS(hf);
  dConsoleRedraw();
  console_changed=0;
  return true;
}


#else
void save_console_state_smem(const char * filename) {
  string state(khicas_state());
  int statesize=state.size();
  // ensure all timers are stopped and uninstalled before calling this function!
  size_t size = statesize+4+sizeof(line_row)*LINE_ROW_MAX + sizeof(int)*4;
  char buffer[sizeof(int)*4];

  memcpy(buffer, &line_index, sizeof(int));
  memcpy(buffer+4, &line_x, sizeof(int));
  memcpy(buffer+8, &line_start, sizeof(int));
  memcpy(buffer+12, &line_count, sizeof(int));

  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char*) filename, strlen(filename)+1);

  // there's no need to delete and create again, because there's no problem
  // if there's junk at the end of the file.
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0) {
    // could not open file. file might not exist yet, or the data folder might not exist at all.
    // try creating both and try opening again
    create_data_folder();
    Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, (int *)&size);
    hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
    if(hFile < 0) return; // if it still fails, there's nothing we can do
  }
  Bfile_WriteFile_OS(hFile, &statesize, sizeof(statesize));
  Bfile_WriteFile_OS(hFile, state.c_str(), statesize);
  Bfile_WriteFile_OS(hFile, buffer, sizeof(buffer));
  Bfile_WriteFile_OS(hFile, line_buf, sizeof(line_row)*LINE_ROW_MAX);
  char cmdhist[N*INPUTBUFLEN+4];
  get_cmd_history(cmdhist);
  int len = strlen(cmdhist);
  // make sure file ends with zeros:
  // (there can be junk at the end of the file):
  cmdhist[len] = 0;
  cmdhist[len+1] = 0;
  cmdhist[len+2] = 0;
  cmdhist[len+3] = 0;
  Bfile_WriteFile_OS(hFile, cmdhist, len+4);
  Bfile_CloseFile_OS(hFile);
}

bool load_console_state_smem(const char * filename) {
  // ensure console memory has been initialized before calling this function!
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1);
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0) {
    return false;
  }
  int statesize;
  // readfile last arg: 0=start of file, -1 current pos
  if (Bfile_ReadFile_OS(hFile, &statesize, sizeof(statesize), 0)!=sizeof(statesize)){
    Bfile_CloseFile_OS(hf);
    return false;    
  }
  char state[statesize+1];
  state[statesize]=0;
  if (Bfile_ReadFile_OS(hFile, state, statesize, -1)!=statesize){
    Bfile_CloseFile_OS(hf);
    return false;
  }
  giac::gen g,ge;
  dconsole_mode=0;
  do_run((char*)state,g,ge);
  dconsole_mode=1;

  char buffer[sizeof(int)*4];
  if (Bfile_ReadFile_OS(hFile, buffer, sizeof(buffer), -1)!=sizeof(buffer)){
    Bfile_CloseFile_OS(hf);
    return false;
  }

  memcpy(&line_index, buffer, sizeof(int));
  memcpy(&line_x, buffer+4, sizeof(int));
  memcpy(&line_start, buffer+8, sizeof(int));
  memcpy(&line_count, buffer+12, sizeof(int));

  Bfile_ReadFile_OS(hFile, line_buf, sizeof(line_row)*LINE_ROW_MAX, -1); // read console scrollback buffer

  // remaining contents (variable length) are the command history
  char cmdhist[N*INPUTBUFLEN] = "";
  Bfile_ReadFile_OS(hFile, cmdhist, N*INPUTBUFLEN, -1);

  unsigned char* src = (unsigned char*)cmdhist;
  unsigned char token[INPUTBUFLEN+1];
  src = toksplit(src, '\n' , token, INPUTBUFLEN);
  while (1) {
    update_cmd_history((char*)token);
    src = toksplit(src, '\n' , token, INPUTBUFLEN);
    if(!token[0]) break;
  }

  Bfile_CloseFile_OS(hFile);
  if(line_buf[line_index][0] == '\x1e') line_x = 0;
  else dConsolePutChar('\n');
  return true;
}
#endif

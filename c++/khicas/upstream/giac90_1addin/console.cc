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
#include "input_lexer.h"
#include "console.h"
#include "catalogGUI.hpp"
//#include "aboutGUI.hpp"
#include "menuGUI.hpp"
#include "inputGUI.hpp"
#include "history.h"
#include "textGUI.hpp"
#include "fileGUI.hpp"
//#include "defs.h"

#include "graphicsProvider.hpp"
#include "fileProvider.hpp"
#include "stringsProvider.hpp"
#include "constantsProvider.hpp"
#include "main.h"
//#define POPUP_PRETTY 1
#define POPUP_PRETTY_STR "Pretty print"


giac::context * contextptr=0;
int xthetat=0,xcas_python_eval=0;

//const console_line data_line={0,0,0,-1,0};
console_line * Line=0;//[LINE_MAX];//={data_line};
char menu_f1[8]={0},menu_f2[8]={0},menu_f3[8]={0},menu_f4[8]={0},menu_f5[8]={0},menu_f6[8];
char session_filename[MAX_FILENAME_SIZE+1]="session";
char * FMenu_entries_name[6]={menu_f1,menu_f2,menu_f3,menu_f4,menu_f5,menu_f6};
location Cursor;
unsigned char *Edit_Line=0;
int Start_Line, Last_Line,editline_cursor;
int Case;
int console_changed=0; // 1 if something new in history
int dconsole_mode=1; // 0 disables dConsole commands

#define Current_Line (Start_Line + Cursor.y)
#define Current_Col (Line[Cursor.y + Start_Line].start_col + Cursor.x)

char xcas_status[64];

static void cascas_get_uk_status_time(int &heure,int &minute){
  giac::get_time(heure,minute);
  heure %= 24;
  if (heure<0)
    heure += 24;
  // KhiCAS exposes calculator RTC ticks only here; keep the strip explicitly UK-labelled.
}

void set_xcas_status(){
  ustl::string status;
  int heure,minute; // minutes rounded
  cascas_get_uk_status_time(heure,minute);
  status += char('0'+heure/10);
  status += char('0'+(heure%10));
  status += ':';
  status += char('0'+(minute/10));
  status += char('0'+(minute%10));
  status += " UK ";
  status += xthetat?"t ":"x ";
  status += "CAS ";
  status += giac::angle_radian(contextptr)?"RAD ":"DEG ";
  if (strlen(session_filename)<=24)
    status += ustl::string(session_filename);
#if 0
  else {
    void * ptr = malloc(1024);
    size_t k=(size_t) ptr ;
    free(ptr);
    // size_t k=(size_t) status.c_str();
    // k &= 0x7fffffff;
    status += giac::hexa_print_INT_((int) k);
  }
#endif
  strcpy(xcas_status,status.c_str());
  DefineStatusMessage(xcas_status, 1, 0, 0);
  DisplayStatusArea();
}

void menu_setup(){
  Menu smallmenu;
  smallmenu.numitems=6;
  MenuItem smallmenuitems[smallmenu.numitems];
  smallmenu.items=smallmenuitems;
  smallmenu.height=8;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  smallmenu.title = (char*)"Config";
  smallmenuitems[0].type = MENUITEM_CHECKBOX;
  smallmenuitems[0].text = (char*)"X,Theta,T=t";
  smallmenuitems[1].type = MENUITEM_CHECKBOX;
  smallmenuitems[1].text = (char*)"Radians";
  smallmenuitems[2].type = MENUITEM_CHECKBOX;
  smallmenuitems[2].text = (char*)"Sqrt";
  smallmenuitems[3].type = MENUITEM_NORMAL;
  smallmenuitems[3].text = (char *) (lang?"Raccourcis":"Shortcuts");
  smallmenuitems[4].type = MENUITEM_NORMAL;
  smallmenuitems[4].text = (char*) (lang?"A propos":"About");
  smallmenuitems[5].type = MENUITEM_NORMAL;
  smallmenuitems[5].text = (char*) "Quit";
  // smallmenuitems[2].text = (char*)(isRecording ? "Stop Recording" : "Record Script");
  while(1) {
    smallmenuitems[0].value = xthetat;
    smallmenuitems[1].value = giac::angle_radian(contextptr);
    smallmenuitems[2].value = giac::withsqrt(contextptr);
    int sres = doMenu(&smallmenu);
    if (sres==MENU_RETURN_EXIT)
      break;
    if (sres == MENU_RETURN_SELECTION) {
      if (smallmenu.selection == 6)
	break;
      if (smallmenu.selection == 1){
	xthetat=1-xthetat;
	continue;
      }
      if (smallmenu.selection == 2){
	giac::angle_radian(!giac::angle_radian(contextptr),contextptr);
	continue;
      }
      if (smallmenu.selection == 3){
	giac::withsqrt(!giac::withsqrt(contextptr),contextptr);
	continue;
      }
      if(smallmenu.selection >= 4) {
	textArea text;
	text.editable=false;
	text.clipline=-1;
	text.title = smallmenuitems[smallmenu.selection-1].text;
	add(&text,smallmenu.selection==4?shortcuts_string:apropos_string);
	doTextArea(&text);
	continue;
      } 
    }	
  }      
}

void * console_malloc(unsigned s){
  return new char [s];
  // return malloc(s);
}

void console_free(void * ptr){
  delete [] (char *) ptr;
  // free(ptr);
}

void delete_clipboard(){}

bool clip_pasted=true;
  
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

int print_msg12(const char * msg1,const char * msg2,int textY){
  drawRectangle(0, textY+10, LCD_WIDTH_PX, 60, COLOR_WHITE);
  drawRectangle(3,textY+10,380,3, COLOR_BLACK);
  drawRectangle(3,textY+10,3,60, COLOR_BLACK);
  drawRectangle(380,textY+10,3,60, COLOR_BLACK);
  drawRectangle(3,textY+70,380,3, COLOR_BLACK);
  int textX=30;
  if (msg1)
    PrintMini(&textX,&textY,(unsigned char*)msg1,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); 
  textX=10;
  textY+=25;
  if (msg2)
    PrintMini(&textX,&textY,(unsigned char*)msg2,0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); 
  return textX;
}

void insert(string & s,int pos,const char * add){
  if (pos>s.size())
    pos=s.size();
  if (pos<0)
    pos=0;
  s=s.substr(0,pos)+add+s.substr(pos,s.size()-pos);
}

bool inputdouble(const char * msg1,double & d){
  ustl::string s1;
  inputline(msg1,lang?"Nouvelle valeur?":"New value?",s1,false);
  return stringtodouble(s1,d);
}

int inputline(const char * msg1,const char * msg2,ustl::string & s,bool numeric,int ypos){
  // s="";
  int pos=s.size(),beg=0;
  for (;;){
    int X1=print_msg12(msg1,msg2,ypos-25);
    int textX=X1,textY=ypos;
    drawRectangle(textX,textY+24,LCD_WIDTH_PX-textX-4,18,COLOR_WHITE);
    if (pos-beg>36)
      beg=pos-12;
    if (int(s.size())-beg<36)
      beg=giac::giacmax(0,int(s.size())-36);
    if (beg>pos)
      beg=pos;
    textX=X1;
    PrintMini(&textX,&textY,(unsigned char *)s.substr(beg,pos-beg).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    int cursorpos=textX;
    PrintMini(&textX,&textY,(unsigned char*)s.substr(pos,s.size()-pos).c_str(),0x02, 0xFFFFFFFF, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
    drawRectangle(cursorpos,textY+24,3,18,COLOR_BLACK); // cursor
    PrintMini(0,58,"         |        |        |        |  A<>a  |       ",4);
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    int key;
    ck_getkey(&key);
    if (!giac::freeze)
      set_xcas_status();    
    if (key==KEY_CTRL_F5){
      handle_f5();
      continue;
    }
    if (key==KEY_CTRL_EXE){
      SetSetupSetting( (unsigned int)0x14,0);
      return key;
    }
    if (key>=32 && key<128){
      if (!numeric || key=='-' || (key>='0' && key<='9'))
	s.insert(s.begin()+pos,char(key));
      ++pos;
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
    if (const char * ans=keytostring(key,keyflag,false,contextptr)){
      insert(s,pos,ans);
      pos+=strlen(ans);
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

int confirm(const char * msg1,const char * msg2,bool acexit){
  int key=0;
  print_msg12(msg1,msg2);
  PrintMini(0,58,"   F1   |        |        |        |       |   F6   ",4);
  while (key!=KEY_CTRL_F1 && key!=KEY_CTRL_F6){
    ck_getkey(&key);
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
	SetSetupSetting( (unsigned int)0x14,0);
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
	    SetSetupSetting( (unsigned int)0x14,0);
	    return "";
	  } // l<256
	}
      } // ge==g || overwrite confirmed
    } // g.type==_IDNT
    else {
      invalid_varname();
    }	
  } // isalpha
  SetSetupSetting( (unsigned int)0x14,0);
  return 0;
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
  Bfile_StrToName_ncpy(pFile, (const unsigned char*)filename, strlen(filename)+1); 
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

const int max_lines_saved=50;

void save_console_state_smem(const char * filename){
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
  int size=2*sizeof(int)+statesize+scriptsize,consolesize=0;
  int start_row=Last_Line-max_lines_saved; 
  if (start_row<0) start_row=0;
  for (int i=start_row;i<=Last_Line;++i){
    int dl=2*sizeof(short)+2*sizeof(char)+strlen((const char *)Line[i].str);
    consolesize += dl;
    size += dl;
  }
  // there's no need to delete and create again, because there's no problem
  // if there's junk at the end of the file.
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0) {
    // could not open file. file might not exist yet, or the data folder might not exist at all.
    // try creating both and try opening again
    //create_data_folder();
    Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size); // Bfile_CreateFile(pFile, size);
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
#if 1
  char consolebuf[consolesize+4];
  char * consoleptr=consolebuf;
  for (int i=start_row;i<=Last_Line;++i){
    console_line & cur=Line[i];
    unsigned short l=strlen((const char *)cur.str);
    memcpy(consoleptr, &l, sizeof(l)); consoleptr += sizeof(l);
    unsigned short s=cur.start_col;
    memcpy(consoleptr, &s, sizeof(s)); consoleptr += sizeof(s);
    unsigned char c=cur.type;
    memcpy(consoleptr, &c, sizeof(c)); consoleptr += sizeof(c);
    c=true; // cur.readonly;
    memcpy(consoleptr, &c, sizeof(c)); consoleptr += sizeof(c);
    unsigned char buf[l+1];
    buf[l]=0;
    strcpy((char *)buf,(const char*)cur.str); 
    unsigned char *ptr=buf,*strend=ptr+l;
    for (;ptr<strend;++ptr){
      if (*ptr==0x9c)
	*ptr='\n';
    }
    memcpy(consoleptr, buf, l); consoleptr+=l;
  }
  char BUF[2]={0,0};
  memcpy(consoleptr, BUF, sizeof(BUF)); consoleptr +=sizeof(BUF);
  Bfile_WriteFile_OS(hFile, consolebuf, consolesize+sizeof(BUF));
#else
  for (int i=start_row;i<=Last_Line;++i){
    console_line & cur=Line[i];
    unsigned short l=strlen((const char *)cur.str);
    Bfile_WriteFile_OS(hFile, &l, sizeof(l));
    unsigned short s=cur.start_col;
    Bfile_WriteFile_OS(hFile, &s, sizeof(s));
    unsigned char c=cur.type;
    Bfile_WriteFile_OS(hFile, &c, sizeof(c));
    c=true; // cur.readonly;
    Bfile_WriteFile_OS(hFile, &c, sizeof(c));
    unsigned char buf[l+1];
    buf[l]=0;
    strcpy((char *)buf,(const char*)cur.str); 
    unsigned char *ptr=buf,*strend=ptr+l;
    for (;ptr<strend;++ptr){
      if (*ptr==0x9c)
	*ptr='\n';
    }
    Bfile_WriteFile_OS(hFile, buf, l);
  }
  char BUF[2]={0,0};
  Bfile_WriteFile_OS(hFile, BUF, sizeof(BUF));
#endif
  Bfile_CloseFile_OS(hFile);
}


int run_session(int start=0){
  std::vector<std::string> v;
  for (int i=start;i<Last_Line;++i){
    if (Line[i].type==LINE_TYPE_INPUT)
      v.push_back((const char *)Line[i].str);
    console_free(Line[i].str);
    Line[i].str=0;
    Line[i].readonly = 0;
    Line[i].type = LINE_TYPE_INPUT;
    Line[i].start_col = 0;
    Line[i].disp_len = 0;
  }
    Line[Last_Line].str=0;
    Last_Line=start;
    if (start<Start_Line)
      Start_Line=start;
    int savestartline=Start_Line;
    Start_Line=Last_Line>LINE_DISP_MAX?Last_Line-LINE_DISP_MAX:0;
    Cursor.x=0;
    Cursor.y=start-Start_Line;
    Line[start].str=Edit_Line;
    Edit_Line[0]=0;
  if (v.empty()) return 0;
  //Console_Init();
  for (int i=0;i<v.size();++i){
    Console_Output((const unsigned char *)v[i].c_str());
    //int j=Last_Line;
    Console_NewLine(LINE_TYPE_INPUT, 1);
    Console_Disp();
    Bdisp_PutDisp_DD();
    // Line[j].type=LINE_TYPE_INPUT;
    run(v[i].c_str(),6); /* show logo and graph but not eqw */
    // j=Last_Line;
    Console_NewLine(LINE_TYPE_OUTPUT, 1);    
    // Line[j].type=LINE_TYPE_OUTPUT;
  }
    int cl=Current_Line;
    Cursor.y += (Start_Line-savestartline);
    if (Cursor.y<0) Cursor.y=0;
    Start_Line=savestartline;
    if (Current_Line>cl || Cursor.y>8){
      if (cl>8){
	Start_Line=cl-8;
	Cursor.y=8;
      }
      else {
	Start_Line=0;
	Cursor.y=cl;
      }
    }
    Console_Disp();
    Bdisp_PutDisp_DD();
  return 0;
}


bool load_console_state_smem(const char * filename){
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  Bfile_StrToName_ncpy(pFile, (const unsigned char *)filename, strlen(filename)+1);
  int hf = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  // cout << hf << endl << "f:" << filename << endl; Console_Disp();
  if (hf < 0) return false; // nothing to load
  // int Bfile_ReadFile(int HANDLE,void *buf,int size,int readpos);
  // read variables and modes
  int L=0;
  if (Bfile_ReadFile_OS(hf,&L,sizeof(L),-1)!=sizeof(L) || L==0){
    Bfile_CloseFile_OS(hf);
    return false;
  }  
  char BUF[L+4];
  BUF[1]=BUF[0]='/'; // avoid trying python compat.
  BUF[2]='\n';
  if (Bfile_ReadFile_OS(hf,BUF+3,L,-1)!=L){
    Bfile_CloseFile_OS(hf);
    return false;
  }
  BUF[L+3]=0;
  giac::gen g,ge;
  dconsole_mode=0;
  xcas_mode(contextptr)=python_compat(contextptr)=0;
  bool bi=try_parse_i(contextptr);
  try_parse_i(false,contextptr);
  try_parse_i(bi,contextptr);
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
  // read console state
  // insure parse messages are cleared
  Console_Init();
  Console_Clear_EditLine();
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
    while (Line[Current_Line].readonly)
      Console_MoveCursor(CURSOR_DOWN);
    Console_Input((const unsigned char *)buf);
    Console_NewLine(LINE_TYPE_INPUT, 1);
#if 1
    if (Current_Line>0){
      console_line & cur=Line[Current_Line-1];
      cur.type=type;
      cur.readonly=readonly;
      cur.start_col+=curs;
    }
#endif
  }
  Bfile_CloseFile_OS(hf);
  console_changed=0;
#if 1
  int p=python_compat(contextptr);
  if (p>=0 && p&4){
    xcas_python_eval=1;
    if (edptr){
      check_parse(edptr->elements,python_compat(contextptr));
    }
  }
  else
    xcas_python_eval=p<0?-1:0;
  if (p==-1){
    //js_ck_eval("1",&global_js_context);
    if (edptr)
      check_parse(edptr->elements,-1);
  }
  Console_FMenu_Init(); // insure the menus are sync-ed
#endif
  return true;
}

#if 0
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
  giac::handle_f5();
  if (giac::inputline(msg.c_str(),(lang?"Nom de variable:":"Variable name:"),*sptr,false) && !sptr->empty() && isalpha((*sptr)[0])){
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
      if (ge==g || giac::confirm_overwrite()){
	*sptr="";
	if (giac::inputline((lang?"Nombre de lignes":"Line number"),"",*sptr,true)){
	  int l=strtol(sptr->c_str(),0,10);
	  if (l>0 && l<256){
	    int c;
	    if (list)
	      c=0;
	    else {
	      ustl::string tmp(*sptr+(lang?" lignes.":" lines."));
	      *sptr="";
	      giac::inputline(tmp.c_str(),lang?"Colonnes:":"Columns:",*sptr,true);
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
      giac::invalid_varname();
    }	
  } // isalpha
  return 0;
}
#endif

/*

  The following functions will be used to specify the location before deleting a string of n characters altogether. Among them, a wide character (2 bytes) will be counted as a character.
	
  For example, we have the following string str:
	
  Location  |  0  |  1  |  2  |  3  |  4  |  5  | 6 |
  Character | 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 0 |

  After the call Console_DelStr (str, 3, 2), position 1 and 2 characters will be deleted, then the characters will be in advance.
	
  Results are as follows:

  Location  |  0  |  1  |  2  |  3  | 4 |  5  | 6 |
  Character | 'a' | 'd' | 'e' | 'f' | 0 | 'f' | 0 |

  (Note: the extra positions will not be filled with '\ 0', but '\ 0' will be a copy of the original end of the string.)

*/

int Console_DelStr(unsigned char *str, int end_pos, int n)
{
  int str_len, actual_end_pos, start_pos, actual_start_pos, del_len, i;

  str_len = strlen((const char *)str);
  if ((start_pos = end_pos - n) < 0) return CONSOLE_ARG_ERR;

  if ((actual_end_pos = Console_GetActualPos(str, end_pos)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
  if ((actual_start_pos = Console_GetActualPos(str, start_pos)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;

  del_len = actual_end_pos - actual_start_pos;

  for (i = actual_start_pos; i < str_len; i++)
    {
      str[i] = str[i + del_len];
    }

  return CONSOLE_SUCCEEDED;
}

/*

  The following functions are used to specify the location of the insertion in the specified string.
  (Note: This refers to the position of the printing position when, rather than the actual position.)
*/

int Console_InsStr(unsigned char *dest, const unsigned char *src, int disp_pos)
{
  int i, ins_len, str_len, actual_pos;

  ins_len = strlen((const char *)src);
  str_len = strlen((const char *)dest);

  actual_pos = Console_GetActualPos(dest, disp_pos);

  if (ins_len + str_len >= EDIT_LINE_MAX) return CONSOLE_MEM_ERR;
  if (actual_pos > str_len) return CONSOLE_ARG_ERR;

  for (i = str_len; i >= actual_pos; i--)
    {
      dest[i + ins_len] = dest[i];
    }

  for (i = 0; i < ins_len; i++)
    {
      unsigned char c=src[i];
      if (c=='\n') c=0x9c;
      dest[actual_pos + i] = (c==0x0a?' ':c);
    }

  return CONSOLE_SUCCEEDED;
}

/*

  The following function is used to determine the true position of the string corresponding to the printing position.
  For example, in the following this string str contains wide characters, the location of the print is as follows:

  Location  | 00  |  01 |   02  |  03  | 04   | 05  | 06  |
  Character | one | two | three | four | five | six | \ 0 |

  The actual storage location is as follows:

  Location | 	00  | 01   |  02  |  03  |  04  |  05  |  06  |  07  |  08  |  09  |  10  |  11  |
  Value 	 | 0xD2 | 0xBB | 0xB6 | 0xFE | 0xC8 | 0xFD | 0xCB | 0xC4 | 0xCE | 0xE5 | 0xC1 | 0xF9 |

  You can find the first four characters 'five' is actually stored in the eighth position.
  So, when you call Console_GetActualPos (str, 4), it will return 8.
*/

int Console_GetActualPos(const unsigned char *str, int disp_pos)
{
  int actual_pos, count;

  for (actual_pos = count = 0; count < disp_pos; count++)
    {
      if (str[actual_pos] == '\0') return CONSOLE_ARG_ERR;

      if (is_wchar(str[actual_pos]))
	{
	  actual_pos += 2;
	}
      else
	{
	  actual_pos++;
	}
    }

  return actual_pos;
}

/*
  The following functions are used to obtain a string of print length, ie, a wide character (2 bytes) recorded as a character.
*/

int Console_GetDispLen(const unsigned char *str)
{
  int i, len;

  for (i = len = 0; str[i]!='\0'; len++)
    {
      if (is_wchar(str[i]))
	{
	  i += 2;
	}
      else
	{
	  i++;
	}
    }

  return len;
}

/*
  The following functions are used to move the cursor.
*/

int Console_MoveCursor(int direction)
{
  switch (direction)
    {
    case CURSOR_UP:
      if (Current_Line==Last_Line)
	editline_cursor=Cursor.x;
      //If you need to operate.
      if ((Cursor.y > 0) || (Start_Line > 0)){
	//If the current line is not read-only, then Edit_Line copy to the current line.
	if (!Line[Current_Line].readonly){
	  if ((Line[Current_Line].str = (unsigned char *)console_malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
	  strcpy((char *)Line[Current_Line].str, (const char *)Edit_Line);
	  Line[Current_Line].disp_len = Console_GetDispLen(Line[Current_Line].str);
	  Line[Current_Line].type = LINE_TYPE_INPUT;
	}
	//If the cursor does not move to the top of, directly move the cursor upward.
	  if (Cursor.y > 0)
	      Cursor.y--;
	  //Otherwise, the number of rows, if the screen's first line is not the first line, then began to show minus one.
	  else {
	    if (Start_Line > 0)
	      Start_Line--;
	  }
	  //End if the horizontal position after moving the cursor over the line, then move the cursor to the end of the line.
	  if (Cursor.x > Line[Current_Line].disp_len){
	    Cursor.x = Line[Current_Line].disp_len;
	  }
	  else {
	    if (Line[Current_Line].disp_len - Line[Current_Line].start_col > COL_DISP_MAX){
	      if (Cursor.x == COL_DISP_MAX)
		Cursor.x = COL_DISP_MAX - 1;
	    }
	  }
	  //If you move the cursor to the line after the first, and the front of the line there is a character does not appear, then move the cursor to position 1.
	  if (Cursor.x == 0 && Line[Current_Line].start_col > 0)
	    Cursor.x = 1;
	  //If the current cursor line is not read-only, then it is a string copy to Edit_Line for editing.
	  if (!Line[Current_Line].readonly&& Line[Current_Line].str){
	    strcpy((char *)Edit_Line, (const char *)Line[Current_Line].str);
	    console_free(Line[Current_Line].str);
	    Line[Current_Line].str = Edit_Line;
	  }
      }
      break;
    case CURSOR_ALPHA_UP:{
      int pos1=Start_Line+Cursor.y;
      Console_MoveCursor(CURSOR_UP);
      int pos2=Start_Line+Cursor.y;
      if (pos1<Last_Line && pos2<Last_Line && pos1!=pos2){
	console_line curline=Line[pos1];
	Line[pos1]=Line[pos2];
	Line[pos2]=curline;
      }
      break;
    }
    case CURSOR_ALPHA_DOWN: {
      int pos1=Start_Line+Cursor.y;
      Console_MoveCursor(CURSOR_DOWN);
      int pos2=Start_Line+Cursor.y;
      if (pos1<Last_Line && pos2<Last_Line && pos1!=pos2){
	console_line curline=Line[pos1];
	Line[pos1]=Line[pos2];
	Line[pos2]=curline;
      }
      break;
    }
    case CURSOR_DOWN:
      if (Current_Line==Last_Line)
	editline_cursor=Cursor.x;
      //If you need to operate.
      if ((Cursor.y < LINE_DISP_MAX - 1) && (Current_Line < Last_Line) || (Start_Line + LINE_DISP_MAX - 1 < Last_Line))
	{
	  //If the current line is not read-only, then Edit_Line copy to the current line.
	  if (!Line[Current_Line].readonly)
	    {
	      if ((Line[Current_Line].str = (unsigned char *)console_malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
	      strcpy((char *)Line[Current_Line].str, (const char *)Edit_Line);
	      Line[Current_Line].disp_len = Console_GetDispLen(Line[Current_Line].str);
	      Line[Current_Line].type = LINE_TYPE_INPUT;
	    }

	  //If the cursor does not move to the bottom, the cursor moves down directly.
	  if (Cursor.y < LINE_DISP_MAX - 1 && Current_Line < Last_Line)
	    {
	      Cursor.y++;
	    }
	  //The number of rows Otherwise, if the last line is not the last line on the screen, it will begin to show a plus.
	  else if (Start_Line + LINE_DISP_MAX - 1 < Last_Line)
	    {
	      Start_Line++;
	    }

	  //If you move the cursor after the end of the horizontal position over the line, then move the cursor to the end of the line.
	  if (Cursor.x > Line[Current_Line].disp_len)
	    {
	      Cursor.x = Line[Current_Line].disp_len;
	    }
	  else if (Line[Current_Line].disp_len - Line[Current_Line].start_col >= COL_DISP_MAX)
	    {
	      if (Cursor.x == COL_DISP_MAX) Cursor.x = COL_DISP_MAX - 1;
	    }

	  //If you move the cursor to the line after the first, and the front of the line there is a character does not appear, then move the cursor to position 1.
	  if (Cursor.x == 0 && Line[Current_Line].start_col > 0) Cursor.x = 1;

	  //If the current cursor line is not read-only, then it is a string copy to Edit_Line for editing.
	  if (!Line[Current_Line].readonly && Line[Current_Line].str)
	    {
	      strcpy((char *)Edit_Line, (const char *)Line[Current_Line].str);
	      console_free(Line[Current_Line].str);
	      Line[Current_Line].str = Edit_Line;
	    }
	}
      break;
    case CURSOR_LEFT:
      if (Line[Current_Line].readonly){
	if (Line[Current_Line].start_col > 0){
	  Line[Current_Line].start_col--;
	}
	break;
      }
      else {
	if (Line[Current_Line].start_col > 0){
	  if (Cursor.x > 1)
	    Cursor.x--;
	  else {
	    int dx=COL_DISP_MAX/3;
	    if (Line[Current_Line].start_col>=dx){
	      Line[Current_Line].start_col-=dx;
	      Cursor.x += dx-1;
	    } else
	      Line[Current_Line].start_col--;
	  }
	  break;
	}
	if (Cursor.x > 0){
	  Cursor.x--;
	  break;
	}
      }
    case CURSOR_SHIFT_RIGHT:
      if (!Line[Current_Line].readonly)
	Cursor.x=min(Line[Current_Line].disp_len,COL_DISP_MAX);
      if (Line[Current_Line].disp_len > COL_DISP_MAX)
	Line[Current_Line].start_col = Line[Current_Line].disp_len - COL_DISP_MAX;
      break;
    case CURSOR_RIGHT:
      if (Line[Current_Line].readonly){
	if (Line[Current_Line].disp_len - Line[Current_Line].start_col > COL_DISP_MAX){
	  Line[Current_Line].start_col++;
	}
	break;
      }
      else {
	if (Line[Current_Line].disp_len - Line[Current_Line].start_col > COL_DISP_MAX){
	  if (Cursor.x < COL_DISP_MAX - 1)
	    Cursor.x++;
	  else {
	    int dx=COL_DISP_MAX/3;
	    if (dx>Cursor.x)
	      dx=Cursor.x;
	    Line[Current_Line].start_col+=dx;
	    Cursor.x -= (dx-1);
	  }
	  break;	  
	}
	if (Cursor.x < Line[Current_Line].disp_len - Line[Current_Line].start_col){
	  Cursor.x++;
	  break;
	}
      }
    case CURSOR_SHIFT_LEFT:
      if (!Line[Current_Line].readonly)
	Cursor.x=0;
      Line[Current_Line].start_col=0;
      break;
    default:
      return CONSOLE_ARG_ERR;
      break;
    }
  return CONSOLE_SUCCEEDED;
}

/*
  The following function is used for input.
  String input to the cursor, the cursor will automatically move.
*/

int Console_Input(const unsigned char *str)
{
  console_changed=1;
  int old_len,i,return_val;

  if (!Line[Current_Line].readonly)
    {
      old_len = Line[Current_Line].disp_len;
      return_val = Console_InsStr(Edit_Line, str, Current_Col);
      if (return_val != CONSOLE_SUCCEEDED) return return_val;
      if ((Line[Current_Line].disp_len = Console_GetDispLen(Edit_Line)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
      for (i = 0; i < Line[Current_Line].disp_len - old_len; i++)
	{
	  Console_MoveCursor(CURSOR_RIGHT);
	}
      return CONSOLE_SUCCEEDED;
    }
  else
    {
      return CONSOLE_ARG_ERR;
    }
}

/*
  The following functions are used to output the string to the current line.
*/

int Console_Output(const unsigned char *str)
{
  console_changed=1;
  int return_val, old_len, i;

  if (!Line[Current_Line].readonly)
    {
      old_len = Line[Current_Line].disp_len;

      return_val = Console_InsStr(Edit_Line, str, Current_Col);
      if (return_val != CONSOLE_SUCCEEDED) return return_val;
      if ((Line[Current_Line].disp_len = Console_GetDispLen(Edit_Line)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
      Line[Current_Line].type = LINE_TYPE_OUTPUT;

      for (i = 0; i < Line[Current_Line].disp_len - old_len; i++)
	{
	  Console_MoveCursor(CURSOR_RIGHT);
	}
      return CONSOLE_SUCCEEDED;
    }
  else
    {
      return CONSOLE_ARG_ERR;
    }
}

void dConsolePut(const char * S){
  if (!dconsole_mode)
    return;
  int l=strlen(S);
  char s[l+1];
  strcpy(s,S);
  for (int i=0;i<l-1;++i){
    if (s[i]=='\n' ||
	s[i]==10)
      s[i]=' ';
  }
  Console_Output((const unsigned char *)s);
  if (l && S[l-1]=='\n')
    Console_NewLine(LINE_TYPE_OUTPUT, 1);
}

void dPuts(const char * s){
  dConsolePut(s);
}

#define PUTCHAR_LEN 35
static char putchar_buf[PUTCHAR_LEN+2];
static int putchar_pos=0;
void dConsolePutChar(const char ch){
  if (!dconsole_mode)
    return;
  if (putchar_pos==PUTCHAR_LEN)
    dConsolePutChar('\n');
  if (ch=='\n'){
    putchar_buf[putchar_pos]='\n';
    putchar_buf[putchar_pos+1]=0;
    putchar_pos=0;
    dConsolePut(putchar_buf);
  }
  else {
    putchar_buf[putchar_pos]=ch;
    ++putchar_pos;
  }
}

/*
  Clear the current output line
*/

void Console_Clear_EditLine()
{
  if(!Line[Current_Line].readonly) {
    Edit_Line[0] = '\0';
    Line[Current_Line].start_col = 0;
    Line[Current_Line].disp_len = 0;
    Cursor.x = 0;
  }
}

/*

  The following functions are used to create a new line.
  Pre_line_type type parameter is used to specify the line, pre_line_readonly parameter is used to specify the line is read-only.
  New_line_type parameter is used to specify the type of the next line, new_line_readonly parameter is used to specify the next line is read-only.
*/

int Console_NewLine(int pre_line_type, int pre_line_readonly)
{
  console_changed=1;
  int i;

  if (strlen((const char *)Edit_Line)||Line[Current_Line].type==LINE_TYPE_OUTPUT)
    {
      //Èç¹ûÒÑ¾­ÊÇËùÄÜ´æ´¢µÄ×îºóÒ»ÐÐ£¬ÔòÉ¾³ýµÚÒ»ÐÐ¡£
      //If this is the last line we can store, delete the first line.
      if (Last_Line == LINE_MAX - 1)
	{
	  for (i = 0; i < Last_Line; i++)
	    {
	      Line[i].disp_len = Line[i + 1].disp_len;
	      Line[i].readonly = Line[i + 1].readonly;
	      Line[i].start_col = Line[i + 1].start_col;
	      Line[i].str = Line[i + 1].str;
	      Line[i].type = Line[i + 1].type;
	    }
	  Last_Line--;

	  if (Start_Line > 0) Start_Line--;
	}

      if (Line[Last_Line].type == LINE_TYPE_OUTPUT && strlen((const char *)Edit_Line) == 0) Console_Output((const unsigned char *)"Done");

      //Edit_Line copy the contents to the last line.

      if ((Line[Last_Line].str = (unsigned char *)console_malloc(strlen((const char *)Edit_Line) + 1)) == NULL) return CONSOLE_MEM_ERR;
      strcpy((char *)Line[Last_Line].str, (const char *)Edit_Line);

      if ((Line[Last_Line].disp_len = Console_GetDispLen(Line[Last_Line].str)) == CONSOLE_ARG_ERR) return CONSOLE_ARG_ERR;
      Line[Last_Line].type = pre_line_type;
      Line[Last_Line].readonly = pre_line_readonly;
      Line[Last_Line].start_col = 0;

      Edit_Line[0] = '\0';

      Last_Line++;

      Cursor.x = 0;

      if ((Last_Line - Start_Line) == LINE_DISP_MAX)
	{
	  Start_Line++;
	}
      else
	{
	  Cursor.y++;
	}

      Line[Last_Line].str = Edit_Line;
      Line[Last_Line].readonly = 0;
      Line[Last_Line].type = LINE_TYPE_INPUT;
      Line[Last_Line].start_col = 0;
      Line[Last_Line].disp_len = 0;

      return CONSOLE_NEW_LINE_SET;
    }
  else
    {
      return CONSOLE_NO_EVENT;
    }
}

void Console_Insert_Line(){
  if (Last_Line>=LINE_MAX-1)
    return;
  for (int i=Last_Line;i>=Current_Line;--i){
    Line[i+1]=Line[i];
  }
  ++Last_Line;
  int i=Current_Line;
  console_line & l=Line[i];
  l.str=(unsigned char *)console_malloc(2);
  strcpy((char *)l.str,"0");
  l.type=Line[i+1].type==LINE_TYPE_INPUT?LINE_TYPE_OUTPUT:LINE_TYPE_INPUT;
  l.start_col=0;
  l.readonly=1;
  l.disp_len=Console_GetDispLen(l.str);
}

/*
  The following function is used to delete a character before the cursor.
*/

int Console_Backspace()
{
  console_changed=1;
  if (Last_Line>0 && Current_Line<Last_Line){
    int i=Current_Line;
    if (Edit_Line==Line[i].str)
      Edit_Line=Line[i+1].str;
    if (Line[i].str){
      copy_clipboard((const char *)Line[i].str,true);
      console_free(Line[i].str);
    }
    for (;i<Last_Line;++i){
      Line[i]=Line[i+1];
    }
    Line[i].readonly = 0;
    Line[i].type = LINE_TYPE_INPUT;
    Line[i].start_col = 0;
    Line[i].disp_len = 0;
    Line[i].str=0;
    --Last_Line;
    if (Start_Line>0)
      --Start_Line;
    else {
      if (Cursor.y>0)
	--Cursor.y;
    }
#if 1
    if (Last_Line==0 && Current_Line==0){ // workaround
      char buf[strlen((const char*)Edit_Line)+1];
      strcpy(buf,(const char*)Edit_Line);
      Console_Init();
      Console_Clear_EditLine();
      if (buf[0])
	Console_Input((const unsigned char *)buf);
      //ustl::string status(giac::print_INT_(Last_Line)+" "+(giac::print_INT_(Current_Line)+" ")+giac::print_INT_(Line[Current_Line].str)+" "+(const char*)Line[Current_Line].str);
      //DefineStatusMessage(status.c_str(),1,0,0);
      //DisplayStatusArea();
    }
#endif
    Console_Disp();
    return CONSOLE_SUCCEEDED;
  }
  int return_val;
  return_val = Console_DelStr(Edit_Line, Current_Col, 1);
  if (return_val != CONSOLE_SUCCEEDED) return return_val;
  Line[Current_Line].disp_len = Console_GetDispLen(Edit_Line);
  return Console_MoveCursor(CURSOR_LEFT);
}

/*
  The following functions are used to deal with the key.
*/

void translate_fkey(int & input_key){
  if (input_key==KEY_CTRL_MIXEDFRAC) input_key=KEY_CTRL_F10;
  if (input_key==KEY_CTRL_FRACCNVRT) input_key=KEY_CTRL_F7;
  if (input_key==KEY_CHAR_LIST) input_key=KEY_CTRL_F9;
  if (input_key==KEY_CHAR_MAT) input_key=KEY_CTRL_F8;
  if (input_key==KEY_CTRL_PRGM) input_key=KEY_CTRL_F12;
  if (input_key==KEY_CTRL_FD) input_key=KEY_CTRL_F11;
  if (input_key==KEY_CHAR_ANGLE) input_key=KEY_CTRL_F13;
  if (input_key==KEY_CHAR_FRAC) input_key=KEY_CTRL_F14;
  if (input_key==KEY_CTRL_CAPTURE) input_key=KEY_CTRL_F15;
}

void chk_clearscreen(){
  drawRectangle(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24, COLOR_WHITE);
  if (confirm(lang?"Effacer l'historique?":"Clear history?",lang?"F1: annuler,   F6: effacer":"F1: cancel,   F6: erase",true)==KEY_CTRL_F6){
    Console_Init();
    Console_Clear_EditLine();
  }    
  Console_Disp();
}

/*
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
  Bdisp_MMPrint(x,y-2,((py && editor<2)?" % ":"int"),0,0xffffffff,0,0,jaune,COLOR_WHITE,1,0);
  x += 20;
  PrintMiniMini( &x, &y,(unsigned char *) ((py && editor<2)?"\\":"sum"), 0,4, 0 ); // red
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
*/

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
  giac::kbd_interrupted=ctrl_c=interrupted=false;
  giac::gen g(giac::_VARS(0,contextptr));
  if (g.type!=giac::_VECT || g._VECTptr->empty())
    return giac::undef;
  giac::vecteur & v=*g._VECTptr;
  MenuItem smallmenuitems[v.size()+4];
  vector<ustl::string> vs(v.size()+1);
  int i,total=0;
  const char typ[]="idzDcpiveSfEsFRmuMwgPF";
  for (i=0;i<v.size();++i){
    vs[i]=v[i].print(contextptr);
    if (v[i].type==giac::_IDNT){
      giac::gen w;
      v[i]._IDNTptr->in_eval(0,v[i],w,contextptr,true);
#if 1
      vector<int> vi(9);
      tailles(w,vi);
      total += vi[8];
      if (vi[8]<400)
	vs[i]+=":="+w.print(contextptr);
      else {
	vs[i] += " ~";
	vs[i] += giac::print_INT_(vi[8]);
	vs[i] += ',';
	vs[i] += typ[w.type];
      }
#else
      if (taille(w,50)<50)
	vs[i]+=": "+w.print(contextptr);
#endif
    }
    smallmenuitems[i].text=(char *) vs[i].c_str();
  }
#ifdef TURTLETAB
  total += giac::syms().capacity()*(sizeof(string)+sizeof(giac::gen)+8)+sizeof(giac::sym_string_tab) +sizeof(giac::context)+contextptr->tabptr->capacity()*(sizeof(const char *)+sizeof(giac::gen)+8)+bytesize(giac::history_in(contextptr))+bytesize(giac::history_out(contextptr));
#else
  total += giac::syms().capacity()*(sizeof(string)+sizeof(giac::gen)+8)+sizeof(giac::sym_string_tab) + giac::turtle_stack().capacity()*sizeof(giac::logo_turtle)+sizeof(giac::context)+contextptr->tabptr->capacity()*(sizeof(const char *)+sizeof(giac::gen)+8)+bytesize(giac::history_in(contextptr))+bytesize(giac::history_out(contextptr));
#endif
  vs[i]="purge(~"+giac::print_INT_(total)+')';
  smallmenuitems[i].text=(char *)vs[i].c_str();
  smallmenuitems[i+1].text=(char *)"assume(";
  smallmenuitems[i+2].text=(char *)"restart ";
  smallmenuitems[i+3].text=(char *)"VARS()";
  Menu smallmenu;
  smallmenu.numitems=v.size()+4; 
  smallmenu.items=smallmenuitems;
  smallmenu.height=8;
  smallmenu.scrollbar=1;
  smallmenu.scrollout=1;
  string title=("Variables "+giac::print_INT_(get_free_memory()));
  smallmenu.title = (char*) title.c_str();
  //MsgBoxPush(5);
  int sres = doMenu(&smallmenu);
  //MsgBoxPop();
  if (sres==KEY_CTRL_DEL && smallmenu.selection<=v.size())
    return giac::symbolic(giac::at_purge,v[smallmenu.selection-1]);
  if (sres!=MENU_RETURN_SELECTION)
    return giac::undef;
  if (smallmenu.selection==1+v.size())
    return giac::string2gen("purge(",false);
  if (smallmenu.selection==2+v.size())
    return giac::string2gen("assume(",false);
  if (smallmenu.selection==3+v.size())
    return giac::string2gen("restart",false);
  if (smallmenu.selection==4+v.size())
    return giac::string2gen("VARS()",false);
  return v[smallmenu.selection-1];
}

const char * keytostring(int key,int keyflag,bool py,const giac::context * contextptr){
#if 0
    char buf[512];
    sprintf(buf,"key %i keyflag %i ",key,keyflag);
    Console_Output(buf);
#endif
  const int textsize=512;
  bool alph=keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
  static char text[textsize];
  if (key>=0x20 && key<=0x7e){
    text[0]=key;
    text[1]=0;
    return text;
  }
  switch (key){
  case KEY_CHAR_PLUS:
    return "+";
  case KEY_CHAR_MINUS:
    return "-";
  case KEY_CHAR_PMINUS:
    return "_";
  case KEY_CHAR_MULT:
    return "*";
  case KEY_CHAR_FRAC: case KEY_CTRL_F14:
    return py?"\\":" % ";
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
  case KEY_CHAR_ANGLE: case KEY_CTRL_F13:
    return "polar_complex(";
  case KEY_CTRL_XTT:
    return xthetat?"t":"x";
  case KEY_CHAR_LN:
    return "ln(";
  case KEY_CHAR_LOG:
    return "log10(";
  case KEY_CHAR_EXPN10:
    return "alog10(";
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
  case KEY_CTRL_MIXEDFRAC: case KEY_CTRL_F10:
    return "limit(";
  case KEY_CTRL_FRACCNVRT: case KEY_CTRL_F7:
    return "exact(";
  case KEY_CTRL_FORMAT:
    return "purge(";
  case KEY_CTRL_FD: case KEY_CTRL_F11:
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
    return "";
  }
  case KEY_CHAR_EXP:
    return "e";
  case KEY_CHAR_ANS:
    return "ans()";
  case KEY_CTRL_INS:
    return ":=";
  case KEY_CTRL_F1:
    if (alph) return ";";
    if (keyflag==1) return "!";
    return py?":":":=";
  case KEY_CTRL_F2:
    return (alph)?"#":(keyflag==1?"<":">");
  case KEY_CTRL_F3:
    return (alph)?(py?"\\":"sum("):(keyflag==1?(py?"%":"integrate("):((!py && xthetat)?"diff(":"'"));
  case KEY_CHAR_MAT: case KEY_CTRL_F8: {
    // const char * ptr=input_matrix(false); if (ptr) return ptr;
    if (showCatalog(text,17)){ 
      return text;
    }
    return "";
  }
  case KEY_CHAR_LIST: case KEY_CTRL_F9: {
    //const char * ptr=input_matrix(true); if (ptr) return ptr;
    if (showCatalog(text,16)){
      return text;
    }
    return "";
  }
  case KEY_CTRL_PRGM: case KEY_CTRL_F12:
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
  case KEY_CTRL_SD:
    if(showCatalog(text,11))
      return text;
    return "";
    // return "\"";
  }
  return 0;
}

const char * keytostring(int key,int keyflag){
  return keytostring(key,keyflag,python_compat(contextptr),contextptr);
}

int Console_Eval(const char * buf){
  int start=Current_Line;
  console_free(Line[start].str);
  Line[start].str=(unsigned char *)console_malloc(strlen(buf)+1);
  strcpy((char *)Line[start].str,buf);
  run_session(start);
  int move_line = Last_Line - start;
  for (int i = 0; i < move_line; i++)
    Console_MoveCursor(CURSOR_UP);
  return CONSOLE_SUCCEEDED;
}

  // back is the number of char that should be deleted before inserting
  string help_insert(const char * cmdline,int & back,bool warn){
    back=0;
    int l=strlen(cmdline);
    char buf[l+1024];
    strcpy(buf,cmdline);
    bool openpar=l && buf[l-1]=='(';
    if (openpar){
      buf[l-1]=0;
      --l;
      ++back;
    }
    for (;l>0;--l){
      if (!isalphanum(buf[l-1]) && buf[l-1]!='_')
	break;
    }
    // cmdname in buf+l
    const char * cmdname=buf+l,*cmdnameorig=cmdname;
    l=strlen(cmdname);
    int res=doCatalogMenu(buf,(char *)"Index",0,cmdname);
    if (!res)
      return "";
    return cmdname+l;
  }
 
  bool console_help_insert(bool warn=true){
    if (!Edit_Line)
      return false;
    char buf[strlen((char *)Edit_Line)+1];
    strcpy(buf,(char *)Edit_Line);
    buf[Line[Current_Line].start_col+Cursor.x]=0;
    int back;
    string s=help_insert(buf,back,warn);
    if (s.empty())
      return false;
    for (int i=0;i<back;++i)
      Console_Backspace();
    Console_Input((const unsigned char *)s.c_str());
    Console_Disp();
    return true;
  }

static char *cascas_skip_space(char *s){
  while (s && *s && isspace((unsigned char)*s))
    ++s;
  return s;
}

static bool cascas_extract_work_expr(char *buf){
  if (!buf)
    return false;
  char *p=cascas_skip_space(buf);
  if (isdigit((unsigned char)p[0]) && p[1]=='.'){
    p=cascas_skip_space(p+2);
    char *q=strchr(p,':');
    if (q)
      p=cascas_skip_space(q+1);
  }
  else {
    char *q=strchr(p,':');
    if (q)
      p=cascas_skip_space(q+1);
  }
  if (p!=buf && *p){
    memmove(buf,p,strlen(p)+1);
    return true;
  }
  return false;
}

int Console_GetKey(){
  int key;
  unsigned int i, move_line, move_col;
  unsigned char tmp_str[2];
  unsigned char *tmp;
  for (;;){
    int keyflag = GetSetupSetting(0x14);
    ck_getkey(&key);
    bool alph=oldalphastate;//keyflag==4||keyflag==0x84||keyflag==8||keyflag==0x88;
    // if (key==30006) OS_InnerWait_ms(1000); // key='6';
    translate_fkey(key);
    if (key==KEY_CTRL_F5){
      handle_f5();
      Console_Disp();
      continue;
    }
    if (key==KEY_CTRL_PASTE)
      return Console_Input((const unsigned char*) paste_clipboard());
    if ( (key >= '0' && key <= '9' ) || (key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')){
      tmp_str[0] = key;
      tmp_str[1] = '\0';
      return Console_Input(tmp_str);
    }
    if ( (key==KEY_CHAR_PLUS || key==KEY_CHAR_MINUS || key==KEY_CHAR_MULT || key==KEY_CHAR_DIV) && Current_Line<Last_Line-1){
      console_line * nxt=&Line[Current_Line];
      if (strncmp((const char *)nxt->str,"parameter([",11)==0)
	Console_MoveCursor(CURSOR_UP);
      nxt=&Line[Current_Line+1];
      if (strncmp((const char *)nxt->str,"parameter([",11)==0){
	giac::gen g((const char *)nxt->str,contextptr);
	if (g.is_symb_of_sommet(giac::at_parameter)){
	  g=g._SYMBptr->feuille;
	  if (g.type==giac::_VECT && g._VECTptr->size()>=5){
	    giac::vecteur & v=*g._VECTptr;
	    if (v[0].type==giac::_IDNT && v[1].type==giac::_DOUBLE_ && v[2].type==giac::_DOUBLE_ && v[3].type==giac::_DOUBLE_ && v[4].type==giac::_DOUBLE_){
	      ustl::string s("assume(");
	      s += v[0]._IDNTptr->id_name;
	      s += "=[";
	      int val=1;
	      if (key==KEY_CHAR_MINUS) val=-1;
	      if (key==KEY_CHAR_MULT) val=5;
	      if (key==KEY_CHAR_DIV) val=-5;
	      s += giac::print_DOUBLE_(v[3]._DOUBLE_val + val*v[4]._DOUBLE_val,3);
	      s += ',';
	      s += giac::print_DOUBLE_(v[1]._DOUBLE_val,3);
	      s += ',';
	      s += giac::print_DOUBLE_(v[2]._DOUBLE_val,3);
	      s += ',';
	      s += giac::print_DOUBLE_(v[4]._DOUBLE_val,3);
	      s += "])";
	      return Console_Eval(s.c_str());
	    }
	  }
	}
      }
    }
    if (key == KEY_CTRL_F3 || ( (key==KEY_CTRL_RIGHT || key==KEY_CTRL_LEFT) && Current_Line<Last_Line) ){
      int l=Current_Line;
      bool graph=strcmp((const char *)Line[l].str,"Graphic object")==0;
      if (graph && l>0) --l;
      char buf[max(512,strlen((const char *)Line[l].str+1))];
      strcpy(buf,(const char *)Line[l].str);
      if (Line[l].type==LINE_TYPE_OUTPUT)
	cascas_extract_work_expr(buf);
      if ( (alph || key==KEY_CTRL_RIGHT) ?textedit(buf):eqws(buf,graph)){
	if (Current_Line==Last_Line){
	  Console_Clear_EditLine();
	  return Console_Input((const unsigned char *)buf);
	}
	else {
#if 1
	  if (Line[l].type==LINE_TYPE_INPUT && l<Last_Line-1 && Line[l+1].type==LINE_TYPE_OUTPUT)
	    return Console_Eval(buf);
	  else {
	    console_free(Line[l].str);
	    Line[l].str=(unsigned char*)console_malloc(strlen(buf)+1);
	    Line[l].disp_len = Console_GetDispLen(Line[l].str);
	    strcpy((char *)Line[l].str,buf);
	  }
#else
	  int x=editline_cursor;
 	  move_line = Last_Line - Current_Line;
	  for (i = 0; i <=move_line; i++) Console_MoveCursor(CURSOR_DOWN);
	  Cursor.x=x;
	  return Console_Input((const unsigned char *)buf);
#endif
	}	  
      }
      Console_Disp();
      continue;
    }
    if (key==KEY_CTRL_F4){
      char buf[512];
      if (!showCatalog(buf,0,0))
	buf[0]=0;
      return Console_Input((const unsigned char*)buf);
    }
    if (key==KEY_CTRL_F6){
#if 1
      Menu smallmenu;
      smallmenu.numitems=6;
      MenuItem smallmenuitems[smallmenu.numitems];
      
      smallmenu.items=smallmenuitems;
      smallmenu.height=8;
      smallmenu.scrollbar=1;
      smallmenu.scrollout=1;
      //smallmenu.title = "KhiCAS";
      smallmenuitems[0].text = (char*)(lang?"Effacer historique":"Clear history");
      smallmenuitems[1].text = (char*)(lang?"Editer matrice":"Matrix editor");
      smallmenuitems[2].text = (char*)"Parameter";
      smallmenuitems[3].text = (char*)"Config shift-SETUP";
      smallmenuitems[4].text = (char *) (lang?"Raccourcis":"Shortcuts");
      smallmenuitems[5].text = (char*) (lang?"A propos":"About");
      // smallmenuitems[2].text = (char*)(isRecording ? "Stop Recording" : "Record Script");
      while(1) {
        int sres = doMenu(&smallmenu);
        if(sres == MENU_RETURN_SELECTION) {
	  const char * ptr=0;
	  if(smallmenu.selection == 1) {
	    chk_restart();
	    Console_Init();
	    Console_Clear_EditLine();
	    break;
          }
          if (smallmenu.selection==2){
	    drawRectangle(0, 0, LCD_WIDTH_PX, LCD_HEIGHT_PX-8, COLOR_WHITE);
	    if (ptr=input_matrix(false)) {
	      return Console_Input((const unsigned char *)ptr);
	    }
	    break;
	  }
	  if (smallmenu.selection == 3){
	    Menu paramenu;
	    paramenu.numitems=6;
	    MenuItem paramenuitems[paramenu.numitems];
	    paramenu.items=paramenuitems;
	    paramenu.height=8;
	    paramenu.title = (char *)"Parameter";
	    char menu_xcur[32],menu_xmin[32],menu_xmax[32],menu_xstep[32],menu_name[16]="name a";
	    static char curname='a';
	    menu_name[5]=curname;
	    ++curname;
	    double pcur=0,pmin=-5,pmax=5,pstep=0.1;
	    ustl::string s;
	    bool doit;
	    for (;;){
	      s="cur "+giac::print_DOUBLE_(pcur,6);
	      strcpy(menu_xcur,s.c_str());
	      s="min "+giac::print_DOUBLE_(pmin,6);
	      strcpy(menu_xmin,s.c_str());
	      s="max "+giac::print_DOUBLE_(pmax,6);
	      strcpy(menu_xmax,s.c_str());
	      s="step "+giac::print_DOUBLE_(pstep,6);
	      strcpy(menu_xstep,s.c_str());
	      paramenuitems[0].text = (char *) "OK";
	      paramenuitems[1].text = (char *) menu_name;
	      paramenuitems[2].text = (char *) menu_xcur;
	      paramenuitems[3].text = (char *) menu_xmin;
	      paramenuitems[4].text = (char *) menu_xmax;
	      paramenuitems[5].text = (char *) menu_xstep;
	      int sres = doMenu(&paramenu);
	      doit = sres==MENU_RETURN_SELECTION;
	      if (doit) {
		ustl::string s1; double d;
		if (paramenu.selection==2){
		  handle_f5();
		  if (inputline(menu_name,lang?"Nouvelle valeur?":"New value?",s1,false)==KEY_CTRL_EXE && s1.size()>0 && isalpha(s1[0])){
		    if (s1.size()>10)
		      s1=s1.substr(0,10);
		    strcpy(menu_name,("name "+s1).c_str());
		  }
		  continue;
		}	
		if (paramenu.selection==3){
		  inputdouble(menu_xcur,pcur);
		  continue;
		}
		if (paramenu.selection==4){
		  inputdouble(menu_xmin,pmin);
		  continue;
		}
		if (paramenu.selection==5){
		  inputdouble(menu_xmax,pmax);
		  continue;
		}
		if (paramenu.selection==6){
		  inputdouble(menu_xstep,pstep);
		  pstep=fabs(pstep);
		  continue;
		}
		// if (paramenu.selection==6) break;
	      } // end menu
	      break;
	    } // end for (;;)
	    if (doit && pmin<pmax && pstep>0){
	      s="assume(";
	      s += (menu_name+5);
	      s += "=[";
	      s += (menu_xcur+4);
	      s += ',';
	      s += (menu_xmin+4);
	      s += ',';
	      s += (menu_xmax+4);
	      s += ',';
	      s += (menu_xstep+5);
	      s += "])";
	      return Console_Input((const unsigned char *)s.c_str());
	    }
	    continue;
	  }
	  if (smallmenu.selection == 4){
	    menu_setup();
	    continue;
	  }
	  if(smallmenu.selection >= 5) {
	    textArea text;
	    text.editable=false;
	    text.clipline=-1;
	    text.title = smallmenuitems[smallmenu.selection-1].text;
	    add(&text,smallmenu.selection==5?shortcuts_string:apropos_string);
	    doTextArea(&text);
	    continue;
          } 
        }
        break;
      } // end while(1)
      Console_Disp();
      return CONSOLE_SUCCEEDED;
#else
      char filename[MAX_FILENAME_SIZE+1];
      //drawRectangle(0, 24, LCD_WIDTH_PX, LCD_HEIGHT_PX-24, COLOR_WHITE);
      if (get_filename(filename))
	edit_script(filename);
      //edit_script(0);
      return CONSOLE_SUCCEEDED;
#endif
    }
    if ( (key >= KEY_CTRL_F1 && key <= KEY_CTRL_F6) 
	 || (key >= KEY_CTRL_F7 && key <= KEY_CTRL_F14) 
	 ){
      return Console_FMenu(key);
    }

    if (key == KEY_CTRL_UP)	return Console_MoveCursor(alph?CURSOR_ALPHA_UP:CURSOR_UP);
    if ( (key == KEY_CTRL_DOWN || key==KEY_CTRL_F10
	  //|| key==KEY_CHAR_FRAC
	  ) && Current_Line==Last_Line && !Line[Current_Line].readonly && Current_Col>0){
      // find cmdname
      console_help_insert(false);
      Console_Disp();
      continue;
      // keytooltip=Console_tooltip(contextptr);
    }
    if (key == KEY_CTRL_DOWN)	return Console_MoveCursor(alph?CURSOR_ALPHA_DOWN:CURSOR_DOWN);
    //if (key == KEY_CTRL_PAGEUP)  return Console_MoveCursor(CURSOR_ALPHA_UP);
    //if (key == KEY_CTRL_PAGEDOWN) return Console_MoveCursor(CURSOR_ALPHA_DOWN);
    if (key == KEY_CTRL_LEFT)	return Console_MoveCursor(CURSOR_LEFT);
    if (key == KEY_CTRL_RIGHT)	return Console_MoveCursor(CURSOR_RIGHT);
    if (key == KEY_SHIFT_LEFT)	return Console_MoveCursor(CURSOR_SHIFT_LEFT);
    if (key == KEY_SHIFT_RIGHT)	return Console_MoveCursor(CURSOR_SHIFT_RIGHT);

    if (key == KEY_CTRL_EXIT){
      if (Last_Line==Current_Line){
	Console_Disp();
      }
      else {
	move_line = Last_Line - Current_Line;
	for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
      }
      return CONSOLE_SUCCEEDED;
    }
    if (key == KEY_CTRL_AC)
      {
	if (Line[Current_Line].readonly){
	  move_line = Last_Line - Current_Line;
	  for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
	  return CONSOLE_SUCCEEDED;
	}
	if (Edit_Line[0]=='\0'){
	  //return Console_Input((const unsigned char *)"restart");
	  chk_clearscreen();
	  continue;
	}
	Edit_Line[0] = '\0';
	Line[Current_Line].start_col = 0;
	Line[Current_Line].type = LINE_TYPE_INPUT;
	Line[Current_Line].disp_len = 0;
	Cursor.x = 0;
	return CONSOLE_SUCCEEDED;
      }

    if (key == KEY_CTRL_INS) {
      if (Current_Line<Last_Line){
	Console_Insert_Line();
	Console_Insert_Line();
      }
      else
	Console_Input((const unsigned char*)":=");
      Console_Disp();
      continue;
    }

    if (key == KEY_CTRL_SETUP) {
      menu_setup();
      Console_Disp();
      continue;
    }

    if (key == KEY_CTRL_EXE){
      if (Current_Line == Last_Line)
	{
	  return Console_NewLine(LINE_TYPE_INPUT, 1);
	}
    }
    if (key == KEY_CTRL_DEL)
      return Console_Backspace();

    if (key == KEY_CHAR_CR && (Current_Line!=Last_Line || Cursor.x==0))
      return Console_NewLine(LINE_TYPE_INPUT, 1);
    if (key == KEY_CTRL_CLIP){
      copy_clipboard((const char *)Line[Current_Line].str);
    }
    if (key==KEY_CTRL_EXE){
      tmp = Line[Current_Line].str;
      
#if 1
      int x=editline_cursor;
      move_line = Last_Line - Current_Line;
      for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
      Cursor.x=x;
      if (Cursor.x>COL_DISP_MAX)
	Line[Last_Line].start_col=Cursor.x-COL_DISP_MAX;
#else
      move_line = Last_Line - Current_Line;
      for (i = 0; i <= move_line; i++) Console_MoveCursor(CURSOR_DOWN);
      move_col = Line[Current_Line].disp_len - Current_Col;
      for (i = 0; i <= move_col; i++) Console_MoveCursor(CURSOR_RIGHT);
#endif
      return Console_Input(tmp);
    }
    const char * ptr=keytostring(key,keyflag,0,contextptr);
    if (ptr)
      return Console_Input((const unsigned char *)ptr);
    
  }
  return CONSOLE_NO_EVENT;
}

static unsigned char* original_cfg=0;

int Console_FMenu(int key){
  const char * s=console_menu(key,original_cfg,0),*ptr=0;
  if (!s){
    //cout << "console " << unsigned(s) << endl;
    return CONSOLE_NO_EVENT;
  }
  if (strcmp("matrix(",s)==0 && (ptr=input_matrix(false)) )
    s=ptr;
  if (strcmp("makelist(",s)==0 && (ptr=input_matrix(true)) )
    s=ptr;
  return Console_Input((const unsigned char *)s);
}

const char * console_menu(int key,int active_app){
  return console_menu(key,original_cfg,active_app);
}

const char * console_menu(int key,unsigned char* cfg_,int active_app){
  unsigned char * cfg=cfg_;
  if (key>=KEY_CTRL_F7 && key<=KEY_CTRL_F16)
    key-=9900;
  int i, matched = 0;
  const char * ret=0;
  const int maxentry_size=64;
  static char console_buf[maxentry_size];
  char temp[maxentry_size],menu1[maxentry_size],menu2[maxentry_size],menu3[maxentry_size],menu4[maxentry_size],menu5[maxentry_size],menu6[maxentry_size],menu7[maxentry_size],menu8[maxentry_size];
  char * tabmenu[8]={menu1,menu2,menu3,menu4,menu5,menu6,menu7,menu8};
  struct FMenu entry = {0,tabmenu,0};
  // char* cfg = (char *)memory_load((char *)"\\\\fls0\\FMENU.cfg");

  while (*cfg) {
    //Get each line
    for(i=0; i<maxentry_size-1 && *cfg && *cfg!='\r' && *cfg!='\n'; i++, cfg++) {
      temp[i] = *cfg;
    }
    temp[i]=0;
    //If starting by 'F' followed by the right number, start filling the structure.
    if (temp[0] == 'F' && temp[1]==(key-KEY_CTRL_F1)+'1'){
      matched = 1;
      continue;
    }
    if (temp[0] == 'F' && temp[1]!=(key-KEY_CTRL_F1)+'0'){
      matched = 0;
      continue;
    }
    //Fill the structure
    if (matched && temp[0] && entry.count<8) {
      strcpy(tabmenu[entry.count], temp);
      entry.count++;
    }
    cfg++;
  }
  if(entry.count > 0) {
    ret = Console_Draw_FMenu(key, &entry,cfg,active_app);
    // cout << "console0 " << (unsigned) ret << endl;
    if (!ret) return ret;
    strcpy(console_buf,ret);
    return console_buf;
  }
  return 0;
}

#if 0
char *Console_Make_Entry(const unsigned char* str)
{
  char* entry = NULL;
  entry = (char*)calloc((strlen((const char *)str)+1), sizeof(unsigned char*));
  if(entry) memcpy(entry, (const char *)str, strlen((const char *)str)+1);

  return entry;
}
#endif

void PrintMini(int x,int y,const char * s,int mode){
  x *=3;
  y *=3;
  PrintMini(&x,&y,(unsigned char *)s,mode,0xFFFFFFFF,0,0,COLOR_BLACK, COLOR_WHITE, 1, 0);
}

//Draws and runs the asked for menu.
const char * Console_Draw_FMenu(int key, struct FMenu* menu,unsigned char * cfg,int active_app)
{
  int i, nb_entries = 0, selector = 0, position_number, position_x, ret, longest = 0;
  int input_key;
  char quick[] = "*: ";
  int quick_len = 2;
  char **entries;
  DISPBOX box,box3;
    
  position_number = key - KEY_CTRL_F1;
  if (position_number<0 || position_number>5)
    position_number=4;
    
  entries  = menu->str;
  nb_entries = menu->count;
    
  for(i=0; i<nb_entries; i++)
    if(strlen(entries[i]) > longest) longest = strlen(entries[i]);

  // screen resolution Graph90 384x(216-24), Graph35 128x64
  // factor 3x3
  position_x = 21*position_number;
  if(position_x + longest*4 + quick_len*4 > 115) position_x = 115 - longest*4 - quick_len*4;
    
  box.left = position_x;
  box.right = position_x + longest*4 + quick_len*4  + 6;
  box.bottom = 63-7;
  box.top = 63-7-nb_entries*7;
  box3.left=3*box.left;
  box3.right=3*box.right;
  box3.bottom=3*box.bottom+24;
  box3.top=3*box.top+24;
  
  drawRectangle(box3.left,box3.top,box3.right-box3.left,box3.bottom-box3.top,COLOR_WHITE);
  drawLine(box3.left, box3.bottom, box3.left, box3.top,COLOR_BLACK);
  drawLine(box3.right, box3.bottom, box3.right, box3.top,COLOR_BLACK);
    
  Cursor_SetFlashOff();
    
  for (;;){
    for(i=0; i<nb_entries; i++) {
      quick[0] = '0'+(i+1);
      PrintMini(3+position_x, box.bottom-7*(i+1), quick, 0);
      PrintMini(3+position_x+quick_len*4, box.bottom-7*(i+1), entries[i], 0);
    }
    PrintMini(3+position_x+quick_len*4,box.bottom-7*(selector+1), entries[selector], 4);
    ck_getkey(&input_key);
    if (input_key == KEY_CTRL_EXIT || input_key==KEY_CTRL_AC) return 0;
    if (input_key == KEY_CTRL_UP && selector < nb_entries-1) selector++;	
    if (input_key == KEY_CTRL_DOWN && selector > 0) selector--;
      
    if (input_key == KEY_CTRL_EXE) return entries[selector];
      
    if (input_key >= KEY_CHAR_1 && input_key < KEY_CHAR_1 + nb_entries) return entries[input_key-KEY_CHAR_1];
      
    translate_fkey(input_key);
      
    if ( active_app==0 &&
	 ((input_key >= KEY_CTRL_F1 && input_key <= KEY_CTRL_F6) ||
	  (input_key >= KEY_CTRL_F7 && input_key <= KEY_CTRL_F12) )
	 ){
      Console_Disp();
      key=input_key;
      return console_menu(key,cfg,active_app);
    }
  } // end while input_key!=EXE/EXIT

  return 0; // never reached
}

/*
  ÒÔÏÂº¯ÊýÓÃÓÚ³õÊ¼»¯¡£
*/

int Console_Init()
{
  console_changed=1;
  int i;
  if (!Line){
    Line=new console_line[LINE_MAX];
    for (i = 0; i < LINE_MAX; i++){
      Line[i].str=0;
    }
  }
  Start_Line = 0;
  Last_Line = 0;

  for (i = 0; i < LINE_MAX; i++){
    if (Line[i].str){
      if (Line[i].str==Edit_Line)
	Edit_Line=0;
      console_free(Line[i].str);
      Line[i].str=0;
    }
    Line[i].readonly = 0;
    Line[i].type = LINE_TYPE_INPUT;
    Line[i].start_col = 0;
    Line[i].disp_len = 0;
  }
  if (Edit_Line)
    console_free(Edit_Line);
  if ((Edit_Line = (unsigned char *)console_malloc(EDIT_LINE_MAX + 1)) == NULL) return CONSOLE_MEM_ERR;
  Edit_Line[0]=0;
  Line[0].str = Edit_Line;

  Cursor.x = 0;
  Cursor.y = 0;

  Case = LOWER_CASE;

  /*for(i = 0; i < 6; i++) {
    FMenu_entries[i].name = NULL;
    FMenu_entries[i].count = 0;
    }*/

  Console_FMenu_Init();

  return CONSOLE_SUCCEEDED;
}

const char conf_standard[] = "F1 alg\nsimplify(\nfactor(\npartfrac(\ntcollect(\ntexpand(\nsum(\nF2 calc\n'\ndiff(\nintegrate(\nlimit(\nseries(\nsolve(\ndesolve(\nrsolve(\nF3 2d\nreserved\nF4 cat\nreserved\nF5 A<>a\nreserved\nF6 poly\nproot(\npcoeff(\nquo(\nrem(\ngcd(\negcd(\nF7 arit\n!\nirem(\nifactor(\ngcd(\nisprime(\nlcm(\nF8 mat\nmatrix(\ndet(\nmatpow(\ntran(\nrref(\ninv(\nF9 stat\nmean(\nmedian(\nstddev(\nquartile1(\nquartile3(\ncorrelation(\nlinear_regression(\nF: prob\nbinomial(\nnormald(\npoisson(\nstudent(\nchisquare(\nF; real\nexact(\napprox(\nfloor(\nceil(\nround(\nsign(\nmax(\nmin(\nF< trig\nsin(\ncos(\ntan(\ntcollect(\ntexpand(\ntlin(\nF= cplx\nabs(\narg(\nre(\nim(\nconj(\ncsolve(\ncfactor(";

// Loads the FMenus' data into memory, from a cfg file
#if 0
char* fmenu_cfg=0;
  // Loads the FMenus' data into memory, from a cfg file
  void Console_FMenu_Init()
  {
    char temp[32] = {'\0'};
#if 0
    if (!fmenu_cfg){
      fmenu_cfg = (char *)conf_standard;
      std::string cfg_s;
      // Does the file exists ?
      if (load_script((char*)"FMENU.cfg",cfg_s)){
	char * ptr=new char[cfg_s.size()+1];
	strcpy(ptr,cfg_s.c_str());
	fmenu_cfg=(char *)ptr;
      }
      if(!fmenu_cfg) {
	save_script((const char *)"FMENU.cfg",conf_standard);
	fmenu_cfg = (char *)conf_standard;
      }
    }
#else
    if (0
	// && xcas_python_eval==1
	){
      //fmenu_cfg=(char *)python_conf_standard;
    }
    else {
      fmenu_cfg=(char *)conf_standard;
    }
#endif
    const char *cfg=fmenu_cfg;
    while(*cfg) {
      //Get each line
      int i;
      for(i=0; i<20 && *cfg && *cfg!='\r' && *cfg!='\n'; i++, cfg++) {
	temp[i] = *cfg;
      }
      temp[i]=0;
      //If starting by 'F', adjust the number and eventually set the name of the menu
      if(temp[0] == 'F' && temp[1]>='1' && temp[1]<='6') {
	int number = temp[1]-'0' - 1;
	if(temp[3] && number<6) {
	  strcpy(FMenu_entries_name[number], (char*)temp+3);
	  //FMenu_entries[number].name[4] = '\0';
	}
      }

      memset(temp, '\0', 20);
      cfg++;
    }
    //free(fmenu_cfg);
  }
#else
void Console_FMenu_Init()
{
  int i, number=0, key, handle;
  unsigned char* tmp_realloc = NULL;
  unsigned char temp[20] = {'\0'};
  if (!original_cfg){
    ustl::string cfg_s;
    // Does the file exists ?
    if (load_script((char*)"\\\\fls0\\FMENU.cfg",cfg_s)){
      char * ptr=new char[cfg_s.size()+1];
      strcpy(ptr,cfg_s.c_str());
      original_cfg=(unsigned char *)ptr;
    }
    if(!original_cfg) {
      save_script((const char *)"\\\\fls0\\FMENU.cfg",conf_standard);
      original_cfg = (unsigned char *)conf_standard;
    }
  }

  unsigned char* cfg=original_cfg;

  while(*cfg) {
    //Get each line
    for(i=0; i<20, *cfg && *cfg!='\r' && *cfg!='\n'; i++, cfg++) {
      temp[i] = *cfg;
    }
    temp[i]=0;
    //If starting by 'F', adjust the number and eventually set the name of the menu
    if(temp[0] == 'F' && temp[1]>='1' && temp[1]<='6') {
      number = temp[1]-'0' - 1;
      if(temp[3]) {
	strcpy(FMenu_entries_name[number], (char*)temp+3);
	//FMenu_entries[number].name[4] = '\0';
      }
    }

    memset(temp, '\0', 20);
    cfg++;
  }
  //free(original_cfg);
}
#endif

/*
  ÒÔÏÂº¯ÊýÓÃÓÚÏÔÊ¾ËùÓÐÐÐ¡£
  ×¢Òâ£ºµ÷ÓÃ¸Ãº¯Êýºó£¬½«Ê×ÏÈÇå¿ÕÏÔ´æ¡£
  The following functions are used to display all lines.
  Note: After calling this function, the first clear the memory.
*/

#ifndef CURSOR
int print_x=0,print_y=0,vfontsize=18,hfontsize=12;
#endif

void locate(int x,int y){
#ifdef CURSOR
  return locate_OS(x,y);
#else
  print_x=(x-1)*hfontsize;
  print_y=(y-1)*vfontsize;
#endif
}

void Cursor_SetPosition(int x,int y){
  return locate(x+1,y+1);
}

void PrintRev(const unsigned char * s,int color=TEXT_COLOR_BLACK){
#ifdef CURSOR
  Print_OS((unsigned char *)s,TEXT_MODE_INVERT,0);
#else
  print(print_x,print_y,(const char *)s,color,true/* revert*/,false,false);
#endif  
}

  int print_color(int print_x,int print_y,const char *s,int color,bool invert,bool minimini,GIAC_CONTEXT){
    int python=python_compat(contextptr);
    const char * src=s;
    char singleword[128];
    bool linecomment=false;
    int couleur=color;
    while (*src && print_y<LCD_WIDTH_PX){
      const char * oldsrc=src;
      if ( (python && *src=='#') ||
	   (!python && *src=='/' && *(src+1)=='/')){
	linecomment=true;
	couleur=4;
      }
      if (linecomment)
	src = (char*)toksplit((unsigned char*)src, ' ', (unsigned char*)singleword, minimini?50:35); //break into words; next word
      else { // skip string (only with delimiters " ")
	if (*src=='"'){
	  for (++src;*src;++src){
	    if (*src=='"' && *(src-1)!='\\')
	      break;
	  }
	  if (*src=='"')
	    ++src;
	  int i=src-oldsrc;
	  strncpy(singleword,oldsrc,i);
	  singleword[i]=0;
	}
	else {
	  size_t i=0;
	  for (;*src==' ';++src){ // skip initial whitespaces
	    ++i;
	  }
	  if (i==0){
	    if (isalpha(*src)){ // skip keyword
	      for (;isalphanum(*src) || *src=='_';++src){
		++i;
	      }
	    }
	    // go to next space or alphabetic char
	    for (;*src;++i,++src){
	      if (*src==' ' || (i && *src>=' ' && *src<='/') || (python && *src=='#') || (!python && *src=='/' && *(src+1)=='/')|| *src=='"' || isalpha(*src))
		break;
	    }
	  }
	  strncpy(singleword,oldsrc,i);
	  singleword[i]=0;
	  if (i==0){
	    puts(src); // free(singleword);
	    return print_x; // FIXME KEY_CTRL_F2;
	  }
	} // end normal case
      } // end else linecomment case
      couleur=linecomment?5:find_color(singleword);
      if (couleur==1) couleur=COLOR_BLUE;
      if (couleur==2) couleur=49432; //was COLOR_YELLOWDARK;
      if (couleur==3) couleur=51712;//33024;
      if (couleur==4) couleur=COLOR_MAGENTA;
      if (couleur==5) couleur=COLOR_GREEN;
      if (linecomment || singleword[0]=='"')
	print(print_x,print_y,singleword,couleur,invert,/*fake*/false,minimini);
      else { // print two parts, commandname in color and remain in black
	char * ptr=singleword;
	if (isalpha(*ptr)){
	  while (isalphanum(*ptr) || *ptr=='_')
	    ++ptr;
	}
	char ch=*ptr;
	*ptr=0;
	print(print_x,print_y,singleword,couleur,invert,/*fake*/false,minimini);
	*ptr=ch;
	print(print_x,print_y,ptr,COLOR_BLACK,invert,/*fake*/false,minimini);
      }
      // ?add a space removed from token
      if( linecomment?*src:*src==' ' ){
	if (*src==' ')
	  ++src;
	print(print_x,print_y," ",COLOR_BLACK,invert,false,minimini);
      }
    }
    return print_x;
  }    

  void print_color(const char *s,int color,bool invert,bool minimini,GIAC_CONTEXT){
    print_x=print_color(print_x,print_y,s,color,invert,minimini,contextptr);
  }

void Print(const unsigned char * s,int color,bool colorsyntax){
#ifdef CURSOR
  Print_OS((unsigned char *)s,TEXT_MODE_NORMAL,0);
#else
  if (!colorsyntax || (strlen((const char *)s)==1 && (s[0]=='>' || s[0]=='<')))
      print(print_x,print_y,(const char *)s,color,false,false,false);
    else
      print_color((const char *)s,color,false,false,giac::context0);
#endif
}

int Console_Disp()
{
  bool minimini=false;
  unsigned int* pBitmap;
  int i, alpha_shift_status;
  DISPBOX ficon;
  int print_y = 0; //pixel y cursor
  int print_y_locate;

  Bdisp_AllClr_VRAM();

  //GetFKeyIconPointer( 0x01BE, &ficon );
  //DisplayFKeyIcon( i, ficon);

  //Reading each "line" that will be printed
  for (i = 0; (i < LINE_DISP_MAX) && (i + Start_Line <= Last_Line); i++){
    console_line & curline=Line[i+Start_Line];
    bool colorsyntax=curline.type == LINE_TYPE_INPUT;
    if (i == Cursor.y){
      // cursor line
      if (curline.type == LINE_TYPE_INPUT || curline.type == LINE_TYPE_OUTPUT && curline.disp_len >= COL_DISP_MAX){
	locate(1, i + 1);
	if (curline.readonly){
#ifdef CURSOR
	  Cursor_SetFlashOff();
#endif
	  PrintRev(curline.str + curline.start_col);
	}
	else 
	  Print(curline.str+curline.start_col+(Cursor.x>COL_DISP_MAX-1?1:0),TEXT_COLOR_BLACK,colorsyntax);
      }
      else {
	locate(COL_DISP_MAX - curline.disp_len + 1, i + 1);
	if (curline.readonly){
#ifdef CURSOR
	  Cursor_SetFlashOff();
#endif
	  PrintRev(curline.str);
	}
	else 
	  Print(curline.str,TEXT_COLOR_BLACK,colorsyntax);
      }

      if (curline.disp_len - curline.start_col > COL_DISP_MAX-1){
#ifdef CURSOR
	locate(COL_DISP_MAX, i + 1);
#else
	print_y=i*vfontsize;
	print_x=LCD_WIDTH_PX-hfontsize;
#endif
	if (curline.readonly){
	  if(curline.disp_len - curline.start_col != COL_DISP_MAX) {
#ifdef CURSOR
	    Cursor_SetFlashOff();
#endif
	    PrintRev((unsigned char *)"\xE6\x9B",COLOR_MAGENTA);
	  }
	} // if cur.readonly
	else if (Cursor.x < COL_DISP_MAX-1){
	  Print((unsigned char *)"\xE6\x9B",COLOR_MAGENTA,colorsyntax);
	}
      }

      if (curline.start_col > 0){
	locate(1, i + 1);	
	if (curline.readonly){
#ifdef CURSOR
	  Cursor_SetFlashOff();
#endif		  
	  PrintRev((unsigned char *)"\xE6\x9A",COLOR_MAGENTA);
	}
	else {
	  Print((unsigned char *)"\xE6\x9A",COLOR_MAGENTA,colorsyntax);
	}
      }

      if (!curline.readonly){
	  int fakestart=curline.start_col+(Cursor.x > COL_DISP_MAX-1?1:0);
	  int fakex,fakey=Cursor.y*vfontsize;
	  string fakes;
	  // parenthese match
	  const char * str=(const char *) curline.str;
	  int pos=Cursor.x+fakestart,pos2;
	  int l=strlen(str);
	  char ch=0;
	  if (pos<l)
	    ch=str[pos];
	  int matchdirection=0,paren=0,crochet=0,accolade=0;
	  if (ch=='(' || ch=='[' || ch=='{')
	    matchdirection=1;
	  if (ch=='}' || ch==']' || ch==')')
	    matchdirection=-1;
	  if (!matchdirection && pos){
	    --pos;
	    ch=str[pos];
	    if (ch=='(' || ch=='[' || ch=='{')
	      matchdirection=1;
	    if (ch=='}' || ch==']' || ch==')')
	      matchdirection=-1;
	  }
	  if (matchdirection){
	    char buf[2]={0,0};
	    bool ok=true;
	    for (pos2=pos;ok && (pos2>=0 && pos2<l);pos2+=matchdirection){
	      ch=str[pos2];
	      if (ch=='(') ++paren;
	      if (ch==')') --paren;
	      if (ch=='[') ++crochet;
	      if (ch==']') --crochet;
	      if (ch=='{') ++accolade;
	      if (ch=='}') --accolade;
	      if (matchdirection>0 && (paren<0 || crochet<0 || accolade<0) )
		ok=false;
	      if (matchdirection<0 && (paren>0 || crochet>0 || accolade>0) )
		ok=false;
	      if (paren==0 && crochet==0 && accolade==0)
		break;
	    }
	    ok = paren==0 && crochet==0 && accolade==0;
	    if (pos>=fakestart){
	      fakex=0;
	      buf[0]=str[pos];
	      fakes=string((const char *)curline.str).substr(fakestart,pos-fakestart);
	      print(fakex,fakey,fakes.c_str(),TEXT_COLOR_BLACK,false,true/* fake*/,minimini); // fake print
	      print(fakex,fakey,buf,ok?COLOR_RED:COLOR_GREEN,true/* revert*/,false,minimini);
	    }
	    if (ok){
	      fakex=0;
	      if (pos2>fakestart){
		fakes=string((const char *)curline.str).substr(fakestart,pos2-fakestart);
		print(fakex,fakey,fakes.c_str(),TEXT_COLOR_BLACK,false,true/* fake*/,false); // fake print
		buf[0]=str[pos2];
		print(fakex,fakey,buf,COLOR_RED,true/* revert*/,false,minimini);
	      }
	    } // end if ok
	  } // end if matchdirection
#ifdef CURSOR
	switch(GetSetupSetting( (unsigned int)0x14)) {
	case 0: 
	  alpha_shift_status = 0;
	  break;
	case 1: //Shift enabled
	  alpha_shift_status = 1;
	  break;
	case 4: case 0x84:	//Alpha enabled
	  alpha_shift_status = 2;
	  break;
	case 8: case 0x88:
	  alpha_shift_status = 4;
	  break;
	default: 
	  alpha_shift_status = 0;
	  break;
	}
	Cursor_SetPosition(Cursor.x, Cursor.y);
	Cursor_SetFlashOn(alpha_shift_status);
	//Cursor_SetFlashStyle(alpha_shift_status); //Potential 2.00 OS incompatibilty (cf Simon's doc)
#else
	//locate(Cursor.x+1,Cursor.y+1);
	//DefineStatusMessage((giac::print_DOUBLE_(Cursor.y,6)+","+giac::print_DOUBLE_(print_y,6)).c_str(),1,0,0);
	//DisplayStatusArea();
	fakes=string((const char *)curline.str).substr(fakestart,Cursor.x);
	fakex=0;fakey=Cursor.y*vfontsize;
	print(fakex,fakey,fakes.c_str(),TEXT_COLOR_BLACK,false,true/* fake*/,false);
	drawRectangle(fakex,24+fakey,2,vfontsize,COLOR_BLACK);
	//drawRectangle(Cursor.x*hfontsize,24+Cursor.y*vfontsize,2,vfontsize,COLOR_BLACK);
#endif
      }
    } // end cursor line
    else {
      bool bigoutput = curline.type==LINE_TYPE_OUTPUT && curline.disp_len>=COL_DISP_MAX-3;
      locate(bigoutput?3:1,i+1);
      if (curline.type==LINE_TYPE_INPUT || bigoutput)
	Print(curline.str + curline.start_col,TEXT_COLOR_BLACK,colorsyntax);
      else {
#ifdef CURSOR
	locate(COL_DISP_MAX - Line[i + Start_Line].disp_len + 1, i + 1);
#else
	print(print_x,print_y,(const char *)curline.str,TEXT_COLOR_BLACK,false,true/*fake*/,false);
	print_x=LCD_WIDTH_PX-print_x;
#endif
	Print(curline.str,TEXT_COLOR_BLACK,colorsyntax);
      }
      if (curline.disp_len - curline.start_col > COL_DISP_MAX){
#ifdef CURSOR
	locate(COL_DISP_MAX, i + 1);
#else
	print_x=LCD_WIDTH_PX-hfontsize;
#endif
	Print((unsigned char *)"\xE6\x9B",COLOR_BLUE,colorsyntax);
      }
      if (curline.start_col > 0){
#ifdef CURSOR
	locate(1, i + 1);
#else
	print_x=0;
#endif
	Print((unsigned char *)"\xE6\x9A",COLOR_BLUE,colorsyntax);
      }      
    } // end non cursor line
  } // end loop on all lines


  string menu(" ");
  menu += string(menu_f1);
  while (menu.size()<6)
    menu += " ";
  menu += "| ";
  menu += string(menu_f2);
  while (menu.size()<13)
    menu += " ";
  menu += lang?"| voir | cmds | A<>a | Fich.":"| view | cmds | A<>a | File ";
  //drawRectangle(0,174,LCD_WIDTH_PX,24,COLOR_BLACK);
  PrintMini(0,58,menu.c_str(),4);

  // status, clock, 
  set_xcas_status();
  Bdisp_PutDisp_DD();
  return CONSOLE_SUCCEEDED;
}

void dConsoleRedraw(){
  Console_Disp();
}

/*
  Draw a popup at the center of the screen containing the str expression drawn in pretty print.
*/
#if defined POPUP_PRETTY && defined TEX
void Console_Draw_TeX_Popup(unsigned char* str, int width, int height)
{
  DISPBOX popup;
  DISPBOX temp;
  unsigned char arrows[4*3] = {0xE6, 0x9A, '\0', 0xE6, 0x9B, '\0', 0xE6, 0x9C, '\0', 0xE6, 0x9D, '\0'};
  int margin = 2, border = 1;
  int scroll_lateral = 0, scroll_lateral_flag = 0, scroll_vertical = 0, scroll_vertical_flag = 0;
  int key;

  if(width > 115) {
    popup.left = 5;
    popup.right = 122;

    scroll_lateral_flag = 1;
  }
  else {
    popup.left = 64 - width/2 - margin - border;
    popup.right = 128 - popup.left;
  }

  if(height > 50) {
    popup.top = 5;
    popup.bottom = 57;

    scroll_vertical_flag = 1;
  }
  else {
    popup.top = 32 - height/2 - margin - border;
    popup.bottom = 64 - popup.top;
  }

  /*temp.left = 0; temp.top = 0; temp.right = 128; temp.bottom = 64;
    Bdisp_ReadArea_VRAM (&temp, vram_copy);*/
	
  while(key != KEY_CTRL_EXIT) {
    Bdisp_AreaClr_VRAM(&popup);
    Bdisp_AreaReverseVRAM(popup.left, popup.top, popup.right, popup.bottom);

    Bdisp_AreaReverseVRAM(popup.left + border, popup.top + border, popup.right - border, popup.bottom - border);

    TeX_drawComplex((char*)str, popup.left+border+margin + scroll_lateral, popup.top+border+margin + scroll_vertical); 

    if(scroll_lateral_flag ||scroll_vertical_flag) {
      temp.left = 0; temp.top = 0; temp.right = popup.left-1; temp.bottom = popup.bottom;
      Bdisp_AreaClr_VRAM(&temp);
      temp.left = 0; temp.top = popup.bottom+1; temp.right = 127; temp.bottom = 63;
      Bdisp_AreaClr_VRAM(&temp);
      temp.left = popup.left-1; temp.top = 0; temp.right = 127; temp.bottom = popup.top-1;
      Bdisp_AreaClr_VRAM(&temp);
      temp.left = popup.right+1; temp.top = popup.top-1; temp.right = 127; temp.bottom = 63;
      Bdisp_AreaClr_VRAM(&temp);

      if(scroll_lateral < 0) PrintMini(1, 30, arrows, 0);
      if(scroll_lateral > -(width - 115)) PrintMini(123, 30, arrows + 3, 0);
      if(scroll_vertical < 0) PrintMini(61, 0, arrows + 6, 0);
      if(scroll_vertical > -(height - 47)) PrintMini(61, 58, arrows + 9, 0);

      Bdisp_DrawLineVRAM(popup.left, popup.top, popup.left, popup.bottom);
      Bdisp_DrawLineVRAM(popup.left, popup.top, popup.right, popup.top);
      Bdisp_DrawLineVRAM(popup.left, popup.bottom, popup.right, popup.bottom);
      Bdisp_DrawLineVRAM(popup.right, popup.top, popup.right, popup.bottom);
    }

    ck_getkey(&key);

    if(scroll_lateral_flag) {
      if(key == KEY_CTRL_LEFT && scroll_lateral < 0) scroll_lateral += 5;
      if(key == KEY_CTRL_RIGHT && scroll_lateral > -(width - 115)) scroll_lateral -= 5;

      if(scroll_lateral > 0) scroll_lateral = 0;
    } 
    if(scroll_vertical_flag) {
      if(key == KEY_CTRL_UP && scroll_vertical < 0) scroll_vertical += 3;
      if(key == KEY_CTRL_DOWN && scroll_vertical > -(height - 47)) scroll_vertical -= 3;

      if(scroll_vertical > 0) scroll_vertical = 0;
    }
  }
}
#endif

/*
  ÒÔÏÂº¯ÊýÓÃÓÚÊäÈëÐÐ£¬³É¹¦ºó½«·µ»Ø¸ÃÐÐµÄ×Ö·û´®¡£
*/

unsigned char *Console_GetLine()
{
  SetSetupSetting( (unsigned int)0x14,0);
  int return_val;
	
  do
    {
      return_val = Console_GetKey();
      Console_Disp();
      if (return_val == CONSOLE_MEM_ERR) return NULL;
    } while (return_val != CONSOLE_NEW_LINE_SET);

  return Line[Current_Line - 1].str;
}

/*
  Simple accessor to the Edit_Line buffer.
*/
unsigned char* Console_GetEditLine()
{
  return Edit_Line;
}

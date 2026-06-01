#include "giacPCH.h"
#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
#include <fxcg/rtc.h>
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "textGUI.hpp"
#include "stringsProvider.hpp"
#include "graphicsProvider.hpp"
#include "catalogGUI.hpp"
#include "fileProvider.hpp"
#include "fileGUI.hpp"
#include <vector>
#include "main.h"
#include "console.h"

textArea * edptr=0;
typedef scrollbar TScrollbar;

// called from editor, return 
int check_parse(const ustl::vector<textElement> & v,int python){
  if (v.empty())
    return 0;
  ustl::string s=merge_area(v);
  giac::python_compat(python,contextptr);
  if (python) s="@@"+s; // force Python translation
  giac::gen g(s,contextptr);
  int lineerr=giac::first_error_line(contextptr);
  if (lineerr){
    char status[256];
    for (int i=0;i<sizeof(status);++i)
      status[i]=0;
    ustl::string tok=giac::error_token_name(contextptr);
    int pos=-1;
    if (lineerr>=1 && lineerr<=v.size()){
      pos=v[lineerr-1].s.find(tok);
      const ustl::string & err=v[lineerr-1].s;
      if (pos>=err.size())
	pos=-1;
      if (python){
	// find 1st token, check if it's def/if/elseif/for/while
	size_t i=0,j=0;
	for (;i<err.size();++i){
	  if (err[i]!=' ')
	    break;
	}
	ustl::string firsterr;
	for (j=i;j<err.size();++j){
	  if (!isalpha(err[j]))
	    break;
	  firsterr += err[j];
	}
	// if there is no : at end set pos=-2
	if (firsterr=="for" || firsterr=="def" || firsterr=="if" || firsterr=="elseif" || firsterr=="while"){
	  for (i=err.size()-1;i>0;--i){
	    if (err[i]!=' ')
	      break;
	  }
	  if (err[i]!=':')
	    pos=-2;
	}
      }
    }
    else {
      lineerr=v.size();
      tok=lang?"la fin":"end";
      pos=0;
    }
    if (pos>=0)
      sprintf(status,lang?"Erreur ligne %i a %s":"Error line %i at %s",lineerr,tok.c_str());
    else
      sprintf(status,lang?"Erreur ligne %i %s":"Error line %i %s",lineerr,(pos==-2?(lang?", : manquant ?":", missing :?"):""));
    DefineStatusMessage(status,1,0,0);
  }
  else {
    giac::set_abort();
    g=eval(g,1,contextptr);
    giac::clear_abort();
    ctrl_c=false;
    giac::kbd_interrupted=interrupted=false;
    check_do_graph(g); // define the function
    DefineStatusMessage((char *)(lang?"Syntaxe correcte":"Parse OK"),1,0,0);
  }
  DisplayStatusArea();    
  return lineerr;
}

void fix_newlines(textArea * edptr){
  edptr->elements[0].newLine=0;
  for (size_t i=1;i<edptr->elements.size();++i)
    edptr->elements[i].newLine=1;
}

int end_do_then(const ustl::string & s){
  // skip spaces from end
  int l=s.size(),i,i0;
  const char * ptr=s.c_str();
  for (i=l-1;i>0;--i){
    if (ptr[i]!=' '){
      if (ptr[i]==':' || ptr[i]=='{')
	return 1;
      if (ptr[i]=='}')
	return -1;
      break;
    }
  }
  if (i>0){
    for (i0=i;i0>=0;--i0){
      if (!isalphanum(ptr[i0]) && ptr[i0]!=';' && ptr[i0]!=':')
	break;
    }
    if (i>i0+2){
      if (ptr[i]==';')
      --i;
      if (ptr[i]==':')
      --i;
    }
    ustl::string keyw(ptr+i0+1,ptr+i+1);
    const char * ptr=keyw.c_str();
    if (strcmp(ptr,"faire")==0 || strcmp(ptr,"do")==0 || strcmp(ptr,"alors")==0 || strcmp(ptr,"then")==0)
      return 1;
    if (strcmp(ptr,"fsi")==0 || strcmp(ptr,"end")==0 || strcmp(ptr,"fi")==0 || strcmp(ptr,"od")==0 || strcmp(ptr,"ftantque")==0 || strcmp(ptr,"fpour")==0 || strcmp(ptr,"ffonction")==0 || strcmp(ptr,"ffunction")==0)
      return -1;
  }
  return 0;
}

void add(textArea *edptr,const ustl::string & s){
  int r=1;
  for (size_t i=0;i<s.size();++i){
    if (s[i]=='\n' || s[i]==char(0x9c))
      ++r;
  }
  edptr->elements.reserve(edptr->elements.size()+r);
  textElement cur;
  cur.lineSpacing=2;
  for (size_t i=0;i<s.size();++i){
    char c=s[i];
    if (c!='\n' && c!=char(0x9c)){
      if (c!=char(0x0d))
	cur.s += c;
      continue;
    }
    string tmp=string(cur.s.begin(),cur.s.end());
    cur.s.swap(tmp);
    edptr->elements.push_back(cur);
    ++edptr->line;
    cur.s="";
  }
  if (cur.s.size()){
    edptr->elements.push_back(cur);
    ++edptr->line;
  }
  fix_newlines(edptr);
}

int find_indentation(const ustl::string & s){
  size_t indent=0;
  for (;indent<s.size();++indent){
    if (s[indent]!=' ')
      break;
  }
  return indent;
}

void add_indented_line(ustl::vector<textElement> & v,int & textline,int & textpos){
  // add line
  v.insert(v.begin()+textline+1,v[textline]);
  ustl::string & s=v[textline].s;
  int indent=find_indentation(s);
  if (!s.empty())
    indent += 2*end_do_then(s);
  //cout << indent << s << ":" << endl;
  if (indent<0)
    indent=0;
  v[textline+1].s=ustl::string(indent,' ')+s.substr(textpos,s.size()-textpos);
  v[textline+1].newLine=1;
  v[textline].s=s.substr(0,textpos);
  ++textline;
  v[textline].nlines=1; // will be recomputed by cursor moves
  textpos=indent;
}

void undo(textArea * text){
  if (text->undoelements.empty())
    return;
  giac::swapint(text->line,text->undoline);
  giac::swapint(text->pos,text->undopos);
  giac::swapint(text->clipline,text->undoclipline);
  giac::swapint(text->clippos,text->undoclippos);
  swap(text->elements,text->undoelements);
}

void set_undo(textArea * text){
  text->changed=true;
  text->undoelements=text->elements;
  text->undopos=text->pos;
  text->undoline=text->line;
  text->undoclippos=text->clippos;
  text->undoclipline=text->clipline;
}

void add_nl(textArea * text,const ustl::string & ins){
  ustl::vector<textElement> & v=text->elements;
  ustl::vector<textElement> w(v.begin()+text->line+1,v.end());
  v.erase(v.begin()+text->line+1,v.end());
  add(text,ins);
  for (size_t i=0;i<w.size();++i)
    v.push_back(w[i]);
  fix_newlines(text);
  text->changed=true;
}

void insert(textArea * text,const char * adds,bool indent){
  size_t n=strlen(adds),i=0;
  if (!n)
    return;
  set_undo(text);
  int l=text->line;
  if (l<0 || l>=text->elements.size())
    return; // invalid line number
  ustl::string & s=text->elements[l].s;
  int ss=int(s.size());
  int & pos=text->pos;
  if (pos>ss)
    pos=ss;
  ustl::string ins=s.substr(0,pos);
  for (;i<n;++i){
    if (adds[i]=='\n' || adds[i]==0x1e) {
      break;
    }
    else {
      if (adds[i]!=char(0x0d))
	ins += adds[i];
    }
  }
  if (i==n){ // no newline in inserted string
    s=ins+s.substr(pos,ss-pos);
    pos += n;
    return;
  }
  ustl::string S(adds+i+1);
  int decal=ss-pos;
  S += s.substr(pos,decal);
  // cout << S << " " << ins << endl;
  s=ins;
  if (indent){
    pos=s.size();
    int debut=0;
    for (i=0;i<S.size();++i){
      if (S[i]=='\n' || S[i]==0x1e){
	add_indented_line(text->elements,text->line,pos);
	// cout << S.substr(debut,i-debut) << endl;
	text->elements[text->line].s += S.substr(debut,i-debut);
	pos = text->elements[text->line].s.size();
	debut=i+1;
      }
    }
    //cout << S << " " << debut << " " << i << S.c_str()+debut << endl;
    add_indented_line(text->elements,text->line,pos);
    text->elements[text->line].s += (S.c_str()+debut);
    fix_newlines(text);
  }
  else 
    add_nl(text,S);
  pos = text->elements[text->line].s.size()-decal;
}

ustl::string merge_area(const ustl::vector<textElement> & v){
  ustl::string s;
  size_t l=0;
  for (size_t i=0;i<v.size();++i)
    l += v[i].s.size()+1;
  s.reserve(l);
  for (size_t i=0;i<v.size();++i){
    s += v[i].s;
    s += '\n';
  }
  return s;
}

bool isalphanum(char c){
  return isalpha(c) || (c>='0' && c<='9');
}

void search_msg(){
  DefineStatusMessage((char *)(lang?"EXE: suivant, AC: annuler":"EXE: next, AC: cancel"),1,0,0);
  DisplayStatusArea();    	    
}  


void show_status(textArea * text,const ustl::string & search,const ustl::string & replace){
  if (text->editable && text->clipline>=0)
    DefineStatusMessage((char *)"PAD: select, CLIP: copy, AC: cancel",1,0,0);
  else {
    ustl::string status;
    int heure,minute;
    giac::get_time(heure,minute);
    status += char('0'+heure/10);
    status += char('0'+(heure%10));
    status += ':';
    status += char('0'+(minute/10));
    status += char('0'+(minute%10));
    if (text->editable){
      status += (xthetat?" t":" x");
      status += text->python?(text->python==2?" Py ^xor ":" Py ^=** "):" Xcas ";
      status += giac::remove_extension(text->filename.c_str()+7);
      status += text->changed?" * ":" - ";
      status += giac::printint(text->line+1);
      status += '/';
      status += giac::printint(text->elements.size());
    }
    if (search.size()){
      status += "EXE: " + search;
      if (replace.size())
	status += "->"+replace;
    }
    DefineStatusMessage((char *)status.c_str(), 1, 0, 0);
  }
  DisplayStatusArea();    
}

bool chk_replace(textArea * text,const ustl::string & search,const ustl::string & replace){
  if (replace.size())
    DefineStatusMessage((char *)(lang?"Remplacer? EXE: Oui, 8 ou N: Non":"Replace? EXE: Yes, 8 or N: No"),1,0,0);
  else
    search_msg();
  DisplayStatusArea();
  for (;;){
    int key;
    ck_getkey(&key);
    if (key==KEY_CHAR_MINUS || key==KEY_CHAR_Y || key==KEY_CHAR_9 || key==KEY_CHAR_O || key==KEY_CTRL_EXE){
      if (replace.size()){
	set_undo(text);
	ustl::string & s = text->elements[text->line].s;
	s=s.substr(0,text->pos-search.size())+replace+s.substr(text->pos,s.size()-text->pos);
	search_msg();
      }
      return true;
    }
    if (key==KEY_CHAR_8 || key==KEY_CHAR_N || key==KEY_CTRL_EXIT){
      search_msg();
      return true;
    }
    if (key==KEY_CTRL_AC){
      show_status(text,search,replace);
      return false;
    }
  }
}

int check_leave(textArea * text){
  if (text->editable && text->filename.size()){
    if (text->changed){
      // save or cancel?
      ustl::string tmp=text->filename;
      tmp=tmp.substr(7,tmp.size()-7);
      if (strcmp(tmp.c_str(),"temp.py")==0){
	if (confirm(lang?"Les modifications seront perdues":"Changes will be lost",lang?"F1: annuler,       F6: tant pis":"F1: cancel,       F6: confirm")==KEY_CTRL_F1)
	  return 2;
	else {
	  return 0;
	}
      }
      tmp += lang?" a ete modifie!":" was modified!";
      if (confirm(tmp.c_str(),lang?"F1: sauvegarder,       F6: tant pis":"F1: save,       F6: discard changes")==KEY_CTRL_F1){
	save_script(text->filename.c_str(),merge_area(text->elements));
	text->changed=false;
	return 1;
      }
      return 0;
    }
    return 1;
  }
  return 0;
}

void print(int &X,int&Y,const char * buf,int color,bool revert,bool fake,bool minimini){
  if(minimini) 
    PrintMiniMini( &X, &Y, (unsigned char *)buf, revert?4:0, color, fake?1:0 );
  else 
    PrintMini(&X, &Y, (unsigned char *)buf, revert?4:0, 0xFFFFFFFF, 0, 0, color, COLOR_WHITE, fake?0:1, 0);
}

void match_print(char * singleword,int delta,int X,int Y,bool match,bool minimini){
  // char buflog[128];sprintf(buflog,"%i %i %s               ",delta,(int)match,singleword);puts(buflog);
  char ch=singleword[delta];
  singleword[delta]=0;
  print(X,Y,singleword,0,false,/* fake*/true,minimini);
  singleword[delta]=ch;
  char buf[4];
  buf[0]=ch;
  buf[1]=0;
  // inverted print: colors are reverted too!
  int color;
  if (minimini)
    color=match?TEXT_COLOR_RED:TEXT_COLOR_GREEN;
  else
    color=match?COLOR_RED:COLOR_GREEN;
  print(X,Y,buf,color,true,/*fake*/false,minimini);
}

bool match(textArea * text,int pos,int & line1,int & pos1,int & line2,int & pos2){
  line2=-1;line1=-1;
  int linepos=text->line;
  const ustl::vector<textElement> & v=text->elements;
  if (linepos<0 || linepos>=v.size()) return false;
  const ustl::string * s=&v[linepos].s;
  int ss=s->size();
  if (pos<0 || pos>=ss) return false;
  char ch=(*s)[pos];
  int open1=0,open2=0,open3=0,inc=0;
  if (ch=='(' || ch=='['
      || ch=='{'
      ){
    line1=linepos;
    pos1=pos;
    inc=1;
  }
  if (
      ch=='}' ||
      ch==']' || ch==')'
      ){
    line2=linepos;
    pos2=pos;
    inc=-1;
  }
  if (!inc) return false;
  bool instring=false;
  for (;;){
    for (;pos>=0 && pos<ss;pos+=inc){
      if ((*s)[pos]=='"' && (pos==0 || (*s)[pos-1]!='\\'))
	instring=!instring;
      if (instring)
	continue;
      switch ((*s)[pos]){
      case '(':
	open1++;
	break;
      case '[':
	open2++;
	break;
      case '{':
	open3++;
	break;
      case ')':
	open1--;
	break;
      case ']':
	open2--;
	break;
      case '}':
	open3--;
	break;
      }
      if (open1==0 && open2==0 && open3==0){
	//char buf[128];sprintf(buf,"%i %i",pos_orig,pos);puts(buf);
	if (inc>0){
	  line2=linepos; pos2=pos;
	}
	else {
	  line1=linepos; pos1=pos;
	}
	return true;
      } // end if
    } // end for pos
    linepos+=inc;
    if (linepos<0 || linepos>=v.size())
      return false;
    s=&v[linepos].s;
    ss=s->size();
    pos=inc>0?0:ss-1;
  } // end for linepos
  return false;
}
#if 0
bool match(const char * s,int pos_orig,const char * & match1,const char * & match2){
  match1=0; match2=0;
  int pos=pos_orig;
  int ss=strlen(s);
  if (pos<0 || pos>=ss) return false;
  char ch=s[pos_orig];
  int open1=0,open2=0,open3=0,inc=0;
  if (ch=='(' || ch=='['
      // || ch=='{'
      ){
    match1=s+pos_orig;
    inc=1;
  }
  if (
      //ch=='}' ||
      ch==']' || ch==')'
      ){
    match2=s+pos_orig;
    inc=-1;
  }
  if (!inc) return false;
  bool instring=false;
  for (pos=pos_orig;pos>=0 && pos<ss;pos+=inc){
    if (s[pos]=='"' && (pos==0 || s[pos-1]!='\\'))
      instring=!instring;
    if (instring)
      continue;
    switch (s[pos]){
    case '(':
      open1++;
      break;
    case '[':
      open2++;
      break;
    case '{':
      open3++;
      break;
    case ')':
      open1--;
      break;
    case ']':
      open2--;
      break;
    case '}':
      open3--;
      break;
    }
    if (open1==0 && open2==0 && open3==0){
      //char buf[128];sprintf(buf,"%i %i",pos_orig,pos);puts(buf);
      if (inc>0)
	match2=s+pos;
      else
	match1=s+pos;
      return true;
    }
  }
  return false;
}
#endif

ustl::string get_selection(textArea * text,bool erase){
  int sel_line1,sel_line2,sel_pos1,sel_pos2;
  int clipline=text->clipline,clippos=text->clippos,textline=text->line,textpos=text->pos;
  if (clipline>=0){
    if (clipline<textline || (clipline==textline && clippos<textpos)){
      sel_line1=clipline;
      sel_line2=textline;
      sel_pos1=clippos;
      sel_pos2=textpos;
    }
    else {
      sel_line1=textline;
      sel_line2=clipline;
      sel_pos1=textpos;
      sel_pos2=clippos;
    }
  }
  ustl::string s(text->elements[sel_line1].s);
  if (erase){
    set_undo(text);
    text->line=sel_line1;
    text->pos=sel_pos1;
    text->elements[sel_line1].s=s.substr(0,sel_pos1)+text->elements[sel_line2].s.substr(sel_pos2,text->elements[sel_line2].s.size()-sel_pos2);
  }
  if (sel_line1==sel_line2){
    return s.substr(sel_pos1,sel_pos2-sel_pos1);
  }
  s=s.substr(sel_pos1,s.size()-sel_pos1)+'\n';
  int sel_line1_=sel_line1;
  for (sel_line1++;sel_line1<sel_line2;sel_line1++){
    s += text->elements[sel_line1].s;
    s += '\n';
  }
  s += text->elements[sel_line2].s.substr(0,sel_pos2);
  if (erase)
    text->elements.erase(text->elements.begin()+sel_line1_+1,text->elements.begin()+sel_line2+1);
  return s;
}

void change_mode(textArea * text,int flag){
  if (bool(text->python)!=bool(flag)){
    text->python=flag;
    python_compat(text->python,contextptr);
    show_status(text,"","");
    warn_python(flag,true);
  }
}  

void display(textArea * text,int & isFirstDraw,int & totalTextY,int & scroll,int & textY){
#ifdef CURSOR  
  Cursor_SetFlashOff();
#endif
  bool editable=text->editable;
  int showtitle = !editable && (text->title != NULL);
  ustl::vector<textElement> & v=text->elements;
  //drawRectangle(text->x, text->y+24, text->width, LCD_HEIGHT_PX-24, COLOR_WHITE);
  // insure cursor is visible
  if (editable && !isFirstDraw){
    int linesbefore=0;
    for (int cur=0;cur<text->line;++cur){
      linesbefore += v[cur].nlines;
    }
    // line begin Y is at scroll+linesbefore*17, must be positive
    if (linesbefore*19+scroll<0)
      scroll = -19*linesbefore;
    linesbefore += v[text->line].nlines;
    // after line Y is at scroll+linesbefore*17
    if (linesbefore*19+scroll>154)
      scroll = 154-19*linesbefore;
  }
  textY = scroll+(showtitle ? 24 : 0)+text->y; // 24 pixels for title (or not)
  int deltax=0;
  if (editable){
    if (v.size()<10){
      deltax=9;
    }
    else {
      if (v.size()<100)
	  deltax=18;
      else
	deltax=27;
    }
  }
  int & clipline=text->clipline;
  int & clippos=text->clippos;
  int & textline=text->line;
  int & textpos=text->pos;
  if (textline<0) textline=0;
  if (textline>=text->elements.size())
    textline=text->elements.size()-1;
  if (textpos<0) textpos=0;
  if (textpos>text->elements[textline].s.size())
    textpos=text->elements[textline].s.size();
  //char bufpos[512];  sprintf(bufpos,"%i,%i:%i,%i       ",textpos,textline,text->elements[textline].s.size(),text->elements.size());  puts(bufpos);
  if (clipline>=0){
    if (clipline>=v.size())
      clipline=-1;
    else {
      if (clippos<0)
	clippos=0;
      if (clippos>=v[clipline].s.size())
	clippos=v[clipline].s.size()-1;
    }
  }
  int line1,line2,pos1=0,pos2=0;
  if (!match(text,text->pos,line1,pos1,line2,pos2) && line1==-1 && line2==-1)
    match(text,text->pos-1,line1,pos1,line2,pos2);
  //char bufpos[512];  sprintf(bufpos,"%i,%i:%i,%i       ",line1,pos1,line2,pos2);  puts(bufpos);
  for (int cur=0;cur < v.size();++cur) {
    const char* src = v[cur].s.c_str();
    if (cur==0){
      int l=v[cur].s.size();
      if (l>=1 && src[0]=='#')
	change_mode(text,1); // text->python=true;
      if (l>=2 && src[0]=='/' && src[1]=='/')
	change_mode(text,0); // text->python=false;
      if (l>=8 && src[0]=='f' && (src[1]=='o' || src[1]=='u') && src[2]=='n' && src[3]=='c' && src[4]=='t' && src[5]=='i' && src[6]=='o' && src[7]=='n')
	change_mode(text,0); // text->python=false;
      if (l>=4 && src[0]=='d' && src[1]=='e' && src[2]=='f' && src[3]==' ')
	change_mode(text,1); // text->python=true;
      drawRectangle(text->x, text->y+24, text->width, LCD_HEIGHT_PX-24, COLOR_WHITE);
    }
    int textX=text->x;
    if(v[cur].newLine) 
      textY=textY+text->lineHeight+v[cur].lineSpacing; 
    if (editable){
      char line_s[16];
      sprint_int(line_s,cur+1);
      PrintMiniMini(&textX, &textY, (unsigned char *)line_s, 0, TEXT_COLOR_PURPLE, 0 );
    }
    textX=text->x+deltax;
    int tlen = v[cur].s.size();
    char singleword[tlen+32];
    // char* singleword = (char*)malloc(tlen+1); // because of this, a single text element can't have more bytes than malloc can provide
    if (cur==textline){
      if (textpos<0 || textpos>tlen)
	textpos=tlen;
      if (tlen==0 && text->editable){ // cursor on empty line
	drawRectangle(textX,textY+22,3,16,COLOR_BLACK);
      }
    }
    bool chksel=false;
    int sel_line1,sel_line2,sel_pos1,sel_pos2;
    if (clipline>=0){
      if (clipline<textline || (clipline==textline && clippos<textpos)){
	sel_line1=clipline;
	sel_line2=textline;
	sel_pos1=clippos;
	sel_pos2=textpos;
      }
      else {
	sel_line1=textline;
	sel_line2=clipline;
	sel_pos1=textpos;
	sel_pos2=clippos;
      }
      chksel=(sel_line1<=cur && cur<=sel_line2);
    }
    const char * match1=0; // matching parenthesis (or brackets?)
    const char * match2=0;
    if (cur==line1)
      match1=v[cur].s.c_str()+pos1;
    else
      match1=0;
    if (cur==line2)
      match2=v[cur].s.c_str()+pos2;
    else
      match2=0;
    // if (cur==textline && !match(v[cur].s.c_str(),textpos,match1,match2) && !match1 && !match2) match(v[cur].s.c_str(),textpos-1,match1,match2);
    // char buf[128];sprintf(buf,"%i %i %i        ",cur,(int)match1,(int)match2);puts(buf);
    const char * srcpos=src+textpos;
    bool minimini=v[cur].minimini;
    int couleur=v[cur].color;
    int nlines=1;
    bool linecomment=false;
    while (*src){
      const char * oldsrc=src;
      if ( (text->python && *src=='#') ||
	   (!text->python && *src=='/' && *(src+1)=='/')){
	linecomment=true;
	couleur=4;
      }
      if (linecomment || !text->editable)
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
	      if (*src==' ' || (i && *src==',') || (text->python && *src=='#') || (!text->python && *src=='/' && *(src+1)=='/')|| *src=='"' || isalpha(*src))
		break;
	    }
	  }
	  strncpy(singleword,oldsrc,i);
	  singleword[i]=0;
	  if (i==0){
    puts(src); // free(singleword);
	    return KEY_CTRL_F2;
	  }
	} // end normal case
      } // end else linecomment case
	// take care of selection
      bool invert=false;
      if (chksel){
	if (cur<sel_line1 || cur>sel_line2)
	  invert=false;
	else {
	  int printpos1=oldsrc-v[cur].s.c_str();
	  int printpos2=src-v[cur].s.c_str();
	  if (cur==sel_line1 && printpos1<sel_pos1 && printpos2>sel_pos1){
	    // cut word in 2 parts: first part not selected
	    src=oldsrc+sel_pos1-printpos1;
	    singleword[sel_pos1-printpos1]=0;
	    printpos2=sel_pos1;
	  }
	  if (cur==sel_line2 && printpos1<sel_pos2 && printpos2>sel_pos2){
	    src=oldsrc+sel_pos2-printpos1;
	    singleword[sel_pos2-printpos1]=0;
	    printpos2=sel_pos2;
	  }
	  // now singleword is totally unselected or totally selected
	  // which one?
	  if (cur==sel_line1){
	    if (cur==sel_line2)
	      invert=printpos1>=sel_pos1 && printpos2<=sel_pos2;
	    else
	      invert=printpos1>=sel_pos1;
	  }
	  else {
	    if (cur==sel_line2)
	      invert=printpos2<=sel_pos2;
	    else
	      invert=true;
	  }
	}
      }
      //check if printing this word would go off the screen, with fake PrintMini drawing:
      int temptextX = 0,temptextY=0;
      print(temptextX,temptextY,singleword,couleur,false,/*fake*/true,minimini);
      if(temptextX<text->width && temptextX + textX > text->width-6) {
	if (editable) PrintMini(&textX, &textY, (unsigned char*)"\xe6\x9b", 0x02, 0xFFFFFFFF, 0, 0, COLOR_MAGENTA, COLOR_WHITE, 1, 0);	  
	//time for a new line
	textX=text->x+deltax;
	textY=textY+text->lineHeight+v[cur].lineSpacing;
	++nlines;
      } //else still fits, print new word normally (or just increment textX, if we are not "on stage" yet)
      if(textY >= -24 && textY < LCD_HEIGHT_PX) {
	temptextX=textX;
	if (editable){
	  couleur=linecomment?5:find_color(singleword);
	  if (!minimini){ // translate colors, not the same for printingg
	    if (couleur==1) couleur=COLOR_BLUE;
	    if (couleur==2) couleur=COLOR_ORANGE;
	    if (couleur==3) couleur=COLOR_BROWN;
	    if (couleur==4) couleur=COLOR_MAGENTA;
	    if (couleur==5) couleur=COLOR_GREEN;
	  }
	  //char ch[32];
	  //sprint_int(ch,couleur);
	  //puts(singleword); puts(ch);
	}
	if (linecomment || !text->editable || singleword[0]=='"')
	  print(textX,textY,singleword,couleur,invert,/*fake*/false,minimini);
	else { // print two parts, commandname in color and remain in black
	  char * ptr=singleword;
	  if (isalpha(*ptr)){
	    while (isalphanum(*ptr) || *ptr=='_')
	      ++ptr;
	  }
	  char ch=*ptr;
	  *ptr=0;
	  print(textX,textY,singleword,couleur,invert,/*fake*/false,minimini);
	  *ptr=ch;
	  print(textX,textY,ptr,COLOR_BLACK,invert,/*fake*/false,minimini);
	}
	// ?add a space removed from token
	if( ((linecomment || !text->editable)?*src:*src==' ') || v[cur].spaceAtEnd){
	  if (*src==' ')
	    ++src;
	  print(textX,textY," ",COLOR_BLACK,invert,false,minimini);
	}
	// ?print cursor, and par. matching
	if (editable){
	  if (match1 && oldsrc<=match1 && match1<src)
	    match_print(singleword,match1-oldsrc,temptextX,textY,
			line2!=-1,
			// match2,
			minimini);
	  if (match2 && oldsrc<=match2 && match2<src)
	    match_print(singleword,match2-oldsrc,temptextX,textY,
			line1!=-1,
			//match1,
			minimini);
	}
	if (editable && cur==textline){
	  if (oldsrc<=srcpos && (srcpos<src || (srcpos==src && textpos==tlen))){
	    if (textpos>=2 && v[cur].s[textpos-1]==' ' && v[cur].s[textpos-2]!=' ' && srcpos-oldsrc==strlen(singleword)+1){ // fix cursor position after space
	      //char ch[512];
	      //sprintf(ch,"%s %i %i %i %i",singleword,strlen(singleword),srcpos-oldsrc,textpos,v[cur].s[textpos-2]);
	      //puts(ch);
	      singleword[srcpos-oldsrc-1]=' ';
	    }
	    singleword[srcpos-oldsrc]=0;
	    print(temptextX,temptextY,singleword,couleur,false,/*fake*/true,minimini);
	    //drawLine(temptextX, textY+14, temptextX, textY-14, COLOR_BLACK);
	    //drawLine(temptextX+1, textY+14, temptextX+1, textY-14, COLOR_BLACK);
	    drawRectangle(temptextX-1,textY+22,3,16,COLOR_BLACK);
	  }
	}
      } // end if testY visible
      else {
	textX += temptextX;
	if(*src || v[cur].spaceAtEnd) textX += 7; // size of a PrintMini space
      }
    }
    // free(singleword);
    v[cur].nlines=nlines;
    if(isFirstDraw) {
      totalTextY = textY+(showtitle ? 0 : 24);
    } else if(textY>LCD_HEIGHT_PX) {
      break;
    }
  } // end main draw loop
  isFirstDraw=0;
  if(showtitle) {
    clearLine(1,1);
    drawScreenTitle((char*)text->title);
  }
  //if (editable)
  if (editable){
    PrintMini(0,58," tests | loops | misc | cmds | A<>a |File ",4);
    //draw_menu(1);
  }
  int scrollableHeight = LCD_HEIGHT_PX-24*(showtitle ? 2 : 1)-text->y;
  //draw a scrollbar:
  if(text->scrollbar) {
    TScrollbar sb;
    sb.I1 = 0;
    sb.I5 = 0;
    sb.indicatormaximum = totalTextY;
    sb.indicatorheight = scrollableHeight;
    sb.indicatorpos = -scroll;
    sb.barheight = scrollableHeight;
    sb.bartop = (showtitle ? 24 : 0)+text->y;
    sb.barleft = text->width - 6;
    sb.barwidth = 6;
    
    Scrollbar(&sb);
  }
}  

bool move_to_word(textArea * text,const ustl::string & s,const ustl::string & replace,int & isFirstDraw,int & totalTextY,int & scroll,int & textY){
  if (!s.size())
    return false;
  int line=text->line,pos=text->pos;
  if (line>=text->elements.size())
    line=0;
  if (pos>=text->elements[line].s.size())
    pos=0;
  for (;line<text->elements.size();++line){
    int p=text->elements[line].s.find(s,pos);
    if (p>=0 && p<text->elements[line].s.size()){
      text->line=line;
      text->clipline=line;
      text->clippos=p;
      text->pos=p+s.size();
      display(text,isFirstDraw,totalTextY,scroll,textY);
      text->clipline=-1;
      return chk_replace(text,s,replace);
    }
    pos=0;
  }
  for (line=0;line<text->line;++line){
    int p=text->elements[line].s.find(s,0);
    if (p>=0 && p<text->elements[line].s.size()){
      text->line=line;
      text->clipline=line;
      text->clippos=p;
      text->pos=p+s.size();
      display(text,isFirstDraw,totalTextY,scroll,textY);
      text->clipline=-1;
      return chk_replace(text,s,replace);
    }
  }
  return false;
}

void textarea_help_insert(textArea * text,int exec){
  string curs=text->elements[text->line].s.substr(0,text->pos);
  if (!curs.empty()){
    int b;
    string adds=help_insert(curs.c_str(),b,exec);
    if (!adds.empty()){
      if (b>0){
	std::string & s=text->elements[text->line].s;
	if (b>text->pos)
	  b=text->pos;
	if (b>s.size())
	  b=s.size();
	s=s.substr(0,text->pos-b)+s.substr(text->pos,s.size()-text->pos);//+s.substr(b,s.size()-b);
      }
      insert(text,adds.c_str(),false);
    }
  }
}

int doTextArea(textArea* text) {
  int scroll = 0;
  int isFirstDraw = 1;
  int totalTextY = 0,textY=0;
  bool editable=text->editable;
  int showtitle = !editable && (text->title != NULL);
  int scrollableHeight = LCD_HEIGHT_PX-24*(showtitle ? 2 : 1)-text->y;
  ustl::vector<textElement> & v=text->elements;
  ustl::string search,replace;
  show_status(text,search,replace);
  if (text->line>=v.size())
    text->line=0;
  display(text,isFirstDraw,totalTextY,scroll,textY);
  while(1) {
    if (text->line>=v.size())
      text->line=0;
    display(text,isFirstDraw,totalTextY,scroll,textY);
    if(text->type == TEXTAREATYPE_INSTANT_RETURN) return 0;
    int keyflag = GetSetupSetting( (unsigned int)0x14);
    int key;
    ck_getkey(&key);
    if (key == KEY_CTRL_SETUP) {
      menu_setup();
      continue;
    }
    if (key!=KEY_CTRL_PRGM && key!=KEY_CHAR_FRAC)
      translate_fkey(key);    
    //char keylog[32];sprint_int(keylog,key); puts(keylog);
    show_status(text,search,replace);
    int & clipline=text->clipline;
    int & clippos=text->clippos;
    int & textline=text->line;
    int & textpos=text->pos;
    if (!editable && key>=KEY_CTRL_F1 && key<=KEY_CTRL_F3)
      return key;
    if (editable){
      if (key==KEY_CTRL_F10){ // mixedfrac key translated
	textarea_help_insert(text,key);
	continue;
      }
      if (key==KEY_CHAR_FRAC && clipline<0){
	if (textline==0) continue;
	ustl::string & s=v[textline].s;
	ustl::string & prev_s=v[textline-1].s;
	int indent=find_indentation(s),prev_indent=find_indentation(prev_s);
	if (!prev_s.empty())
	  prev_indent += 2*end_do_then(prev_s);
	int diff=indent-prev_indent; 
	if (diff>0 && diff<=s.size())
	  s=s.substr(diff,s.size()-diff);
	if (diff<0)
	  s=string(-diff,' ')+s;
	textpos -= diff;
	continue;
      }
      if (key==KEY_CTRL_VARS){
	displaylogo();
	continue;
      }
      if (key==KEY_CHAR_ANS){
	int err=check_parse(v,text->python);
	if (err) // move cursor to the error line
	  textline=err-1;
	continue;
      }
      if (key==KEY_CTRL_CLIP) {
#if 1
	if (clipline>=0){
	  copy_clipboard(get_selection(text,false),true);
	  clipline=-1;
	}
	else {
	  show_status(text,search,replace);
	  clipline=textline;
	  clippos=textpos;
	}
#else
	copy_clipboard(v[textline].s,false);
	DefineStatusMessage((char*)"Line copied to clipboard", 1, 0, 0);
	DisplayStatusArea();
#endif
	continue;
      }
      if (key==KEY_CTRL_F5){
	handle_f5();
	continue;
      }
      if (clipline<0){
	const char * adds;
	if ( (key>=KEY_CTRL_F1 && key<=KEY_CTRL_F3) ||
	     (key >= KEY_CTRL_F7 && key <= KEY_CTRL_F14)
	     ){
	  string le_menu="F1 alg\nsolve(\nfactor(\nexpand(\nsubst(\nF2 calc\ndiff(\nintegrate(\nlimit(\nsum(\nF3 misc\n;\n:\n_\n!\n";
	  le_menu += "F7 alg\npartfrac(\nrange(\nxform(\nF8 trig\nsin(\ncos(\ntan(\nsec(\nF9 log\nln(\nlog(\nexp(\nF: vec\nnorm(\ndot(\nF; real\nexact(\napprox(\nfloor(\nceil(\nround(\nF< sym\n<\n>\n=\nF= help\nhelp(\nF> misc\npi\ninf\nsqrt(";
	  const char * ptr=console_menu(key,(unsigned char*)(le_menu.c_str()),2);
	  if (!ptr){
	    show_status(text,search,replace);
	    continue;
	  }
	  adds=ptr;
	}
	else
	  adds=keytostring(key,keyflag,text->python,contextptr);
	if (adds){
	  bool isex=adds[0]=='\n';
	  if (isex)
	    ++adds;
	  bool isif=strcmp(adds,"if ")==0,
	    iselse=strcmp(adds,"else ")==0,
	    isfor=strcmp(adds,"for ")==0,
	    isforin=strcmp(adds,"for in")==0,
	    isdef=(strcmp(adds,"f(x):=")==0 || strcmp(adds,"def")==0),
	    iswhile=strcmp(adds,"while ")==0,
	    islist=strcmp(adds,"list ")==0,
	    ismat=strcmp(adds,"matrix ")==0;
	  if (islist){
	    input_matrix(true);
	    continue;
	  }
	  if (ismat){
	    input_matrix(false);
	    continue;
	  }
	  if (text->python){
	    if (isif)
	      adds=isex?"if x<0:\nx=-x":"if :\n";
	    if (iselse)
	      adds="else:\n";
	    if (isfor)
	      adds=isex?"for j in range(10):\nprint(j*j)":"for  in range():\n";
	    if (isforin)
	      adds=isex?"for j in [1,4,9,16]:\nprint(j)":"for  in :\n";
	    if (iswhile)
	      adds=isex?"a,b=25,15\nwhile b!=0:\na,b=b,a%b":"while :\n";
	    if (isdef)
	      adds=isex?"def f(x):\nreturn x*x*x\n":"def f(x):\n\nreturn\n";
	  } else {
	    if (isif)
	      adds=lang?(isex?"si x<0 alors x:=-x; fsi;":"si  alors\n\nsinon\n\nfsi;"):(isex?"if x<0 then x:=-x; fi;":"if  then\n\nelse\n\nfi;");
	    if (lang && iselse)
	      adds="sinon ";
	    if (isfor)
	      adds=lang?(isex?"pour j de 1 jusque 10 faire\nprint(j*j);\nfpour;":"pour  de  jusque  faire\n\nfpour;"):(isex?"for j from 1 to 10 do\nprint(j*j);\nod;":"for  from  to  do\n\nod;");
	    if (isforin)
	      adds=lang?(isex?"pour j in [1,4,9,16] faire\nprint(j)\nfpour;":"pour  in  faire\n\nfpour;"):(isex?"for j in [1,4,9,16] do\nprint(j);od;":"for  in  do\n\nod;");
	    if (iswhile)
	      adds=lang?(isex?"a,b:=25,15;\ntantque b!=0 faire\na,b:=b,irem(a,b);\nftantque;a;":"tantque  faire\n\nftantque;"):(isex?"a,b:=25,15;\nwhile b!=0 do\na,b:=b,irem(a,b);\nod;a;":"while  do\n\nod;");
	    if (isdef)
	      adds=lang?(isex?"fonction f(x)\nlocal j;\nj:=x*x;\nreturn j;\nffonction:;\n":"fonction f(x)\nlocal j;\n\nreturn ;\nffonction:;"):(isex?"function f(x)\nlocal j;\nj:=x*x;\nreturn j;\nffunction:;\n":"function f(x)\n  local j;\n\n return ;\nffunction:;");
	  }
	  insert(text,adds,key!=KEY_CTRL_PASTE); // was true, but we should not indent when pasting
	  show_status(text,search,replace);
	  continue;
	}
      }
    }
    textElement * ptr=& v[textline];
    const int interligne=16;
    switch(key){
    case KEY_CTRL_DEL:
      if (clipline>=0){
	copy_clipboard(get_selection(text,true),true);
	// erase selection
	clipline=-1;
      }
      else {
	if (editable){
	  if (textpos){
	    set_undo(text);
	    ustl::string & s=v[textline].s;
	    int nextpos=textpos-1;
	    if (textpos==find_indentation(s)){
	      for (int line=textline-1;line>=0;--line){
		int ind=find_indentation(v[line].s);
		if (textpos>ind){
		  nextpos=ind;
		  break;
		}
	      }
	    }
	    s.erase(s.begin()+nextpos,s.begin()+textpos);
	    textpos=nextpos;
	  }
	  else {
	    if (textline){
	      set_undo(text);
	      --textline;
	      textpos=v[textline].s.size();
	      v[textline].s += v[textline+1].s;
	      v[textline].nlines += v[textline+1].nlines;
	      v.erase(v.begin()+textline+1);
	    }
	  }
	}
	show_status(text,search,replace);
      }
      break;
    case KEY_CTRL_EXE:
      if(text->allowEXE) return TEXTAREA_RETURN_EXE;
      if (search.size()){
	for (;;){
	  if (!move_to_word(text,search,replace,isFirstDraw,totalTextY,scroll,textY))
	    break;
	}
	show_status(text,search,replace);
	continue;
      }
      else {
	int err=check_parse(v,text->python);
	if (err) // move cursor to the error line
	  textline=err-1;
	continue;
      }
      break;
    case KEY_CHAR_CR: 
      if (clipline<0 && editable){
	set_undo(text);
	add_indented_line(v,textline,textpos);
	show_status(text,search,replace);
      }
      break;
    case KEY_CTRL_UNDO:
      undo(text);
      break;
    case KEY_SHIFT_LEFT:
      textpos=0;
      break;
    case KEY_SHIFT_RIGHT:
      textpos=v[textline].s.size();
      break;
    case KEY_CTRL_LEFT:
      if (editable){
	--textpos;
	if (textpos<0){
	  if (textline==0)
	    textpos=0;
	  else {
	    --textline;
	    show_status(text,search,replace);
	    textpos=v[textline].s.size();
	  }
	}
	if (textpos>=0)
	  break;
      }
    case KEY_CTRL_UP:
      if (editable){
	if (textline>0){
	  --textline;
	  show_status(text,search,replace);
	}
	else {
	  textline=0;
	  textpos=0;
	}
      } else {
	if (scroll < 0) {
	  scroll = scroll + interligne;
	  if(scroll > 0) scroll = 0;
	}
      }
      break;
    case KEY_CTRL_RIGHT:
      ++textpos;
      if (textpos<=ptr->s.size())
	break;
      if (textline==v.size()-1){
        textpos=ptr->s.size();
	break;
      }
      textpos=0;
    case KEY_CTRL_DOWN:
      if (editable){
        if (textline<v.size()-1)
          ++textline;
	else {
	  textline=v.size()-1;
	  textpos=v[textline].s.size();
	}
	show_status(text,search,replace);
      }
      else {
        if (textY > scrollableHeight-(showtitle ? 0 : interligne)) {
          scroll = scroll - interligne;
          if(scroll < -totalTextY+scrollableHeight-(showtitle ? 0 : interligne)) scroll = -totalTextY+scrollableHeight-(showtitle ? 0 : interligne);
        }
      }
      break;
    case KEY_CTRL_PAGEDOWN:
      if (editable){
        textline=v.size()-1;
	textpos=v[textline].s.size();
      }
      else {
        if (textY > scrollableHeight-(showtitle ? 0 : interligne)) {
	  scroll = scroll - scrollableHeight;
	  if(scroll < -totalTextY+scrollableHeight-(showtitle ? 0 : interligne)) scroll = -totalTextY+scrollableHeight-(showtitle ? 0 : interligne);
        }
      }
      break;
    case KEY_CTRL_PAGEUP:
      if (editable)
        textline=0;
      else {
        if (scroll < 0) {
          scroll = scroll + scrollableHeight;
	  if(scroll > 0) scroll = 0;
        }
      }
      break;
    case KEY_CTRL_F1:
      if(text->allowF1) return KEY_CTRL_F1;
      break;
    case KEY_CTRL_F6:	
      if (clipline<0 && text->editable && text->filename.size()){
	Menu smallmenu = {};
	smallmenu.numitems=11;
	MenuItem smallmenuitems[11] = {};
	smallmenu.items=smallmenuitems;
	smallmenu.height=8;
	smallmenu.scrollbar=0;
	//smallmenu.title = "KhiCAS";
	smallmenuitems[0].text = (char*)lang?"Tester syntaxe":"Check syntax";
	smallmenuitems[1].text = (char*)lang?"Sauvegarder":"Save";
	smallmenuitems[2].text = (char*)lang?"Sauvegarder comme":"Save as";
	smallmenuitems[3].text = (char*)lang?"Inserer":"Insert";
	smallmenuitems[4].text = (char*)lang?"Effacer":"Clear";
	smallmenuitems[5].text = (char*)lang?"Chercher,remplacer":"Search, replace";
	smallmenuitems[6].text = (char*)lang?"Aller a la ligne":"Goto line";
	smallmenuitems[7].type = MENUITEM_CHECKBOX;
	smallmenuitems[7].text = (char*)"Python";
	smallmenuitems[7].value = text->python;
	smallmenuitems[8].text = (char*)lang?"Quitter":"Quit";
	smallmenuitems[9].text = (char *)aide_khicas_string;
	smallmenuitems[10].text = "A propos";
        int sres = doMenu(&smallmenu);
        if(sres == MENU_RETURN_SELECTION) {
	  sres=smallmenu.selection;
	  if(sres >= 10) {
	    textArea text;
	    text.editable=false;
	    text.clipline=-1;
	    text.title = smallmenuitems[sres-1].text;
	    add(&text,smallmenu.selection==10?shortcuts_string:apropos_string);
	    doTextArea(&text);
	    continue;
          }
	  if (sres==1){
	    int err=check_parse(v,text->python);
	    if (err) // move cursor to the error line
	      textline=err-1;
	  } 
	  if (sres==3){
	    char filename[MAX_FILENAME_SIZE+1];
	    if (get_filename(filename,".py")){
	      text->filename=filename;
	      sres=2;
	    }
	  }
          if(sres == 2) {
	    save_script(text->filename.c_str(),merge_area(v));
	    text->changed=false;
	    char status[256];
	    sprintf(status,lang?"%s sauvegarde":"%s saved",text->filename.c_str()+7);
	    DefineStatusMessage(status, 1, 0, 0);
	    DisplayStatusArea();    	    
	  }
	  if (sres==4){
	    char filename[MAX_FILENAME_SIZE+1];
	    ustl::string ins;
	    if (fileBrowser(filename, (char*)"*.py", "Scripts") && load_script(filename,ins))
	      insert(text,ins.c_str(),false);//add_nl(text,ins);
	  }
	  if (sres==5){
	    ustl::string s(merge_area(v));
#if 0
	    for (size_t i=0;i<s.size();++i){
	      if (s[i]=='\n')
		s[i]=0x1e;
	    }
	    CLIP_Store(s.c_str(),s.size()+1);
#endif
	    copy_clipboard(s);
	    set_undo(text);
	    v.resize(1);
	    v[0].s="";
	    textline=0;
	  }
	  if (sres==6){
	    display(text,isFirstDraw,totalTextY,scroll,textY);
	    search=get_searchitem(replace);
	    if (!search.empty()){
	      for (;;){
		if (!move_to_word(text,search,replace,isFirstDraw,totalTextY,scroll,textY)){
		  break;
		}
	      }
	      show_status(text,search,replace);
	    }
	  }
	  if (sres==7){
	    display(text,isFirstDraw,totalTextY,scroll,textY);
	    int l=get_line_number(lang?"Negatif: en partant de la fin":"Negative: counted from the end",lang?"Numero de ligne:":"Line number:");
	    if (l>0)
	      text->line=l-1;
	    if (l<0)
	      text->line=v.size()+l;
	  }
	  if (sres==8){
	    text->python=text->python?0:1;
	    show_status(text,search,replace);
	    python_compat(text->python,contextptr);
	    warn_python(text->python);
	    PrintMini(0,58," tests | loops | misc | cmds | A<>a |File ",4);
	  }
	  if (sres==9){
	    int res=check_leave(text);
	    if (res==2)
	      continue;
	    return TEXTAREA_RETURN_EXIT;
	  }
	}
      }
      break;
    case KEY_CTRL_F2:
      if (clipline<0)
	return KEY_CTRL_F2;
    case KEY_CTRL_EXIT:
      if (clipline>=0){
	clipline=-1;
	show_status(text,search,replace);
	continue;
      }
      if (check_leave(text)==2)
	continue;
      return TEXTAREA_RETURN_EXIT;
    case KEY_CTRL_INS:
      break;
    default:
      if (clipline<0 && key>=32 && key<128 && editable){
	char buf[2]={char(key),0};
	insert(text,buf,false);
	show_status(text,search,replace);
      }
      if (key==KEY_CTRL_AC){
	if (clipline>=0){
	  clipline=-1;
	  show_status(text,search,replace);
	}
	else {
	  if (search.size()){
	    search="";
	    show_status(text,search,replace);
	  }
	  else {
	    copy_clipboard(v[textline].s+'\n');
	    if (v.size()==1)
	      v[0].s="";
	    else {
	      v.erase(v.begin()+textline);
	      if (textline>=v.size())
		--textline;
	    }
	    DefineStatusMessage((char*)"Line cut and copied to clipboard", 1, 0, 0);
	    DisplayStatusArea();
	  }
	}
      }
    }
  }
}

void reload_edptr(const char * filename,textArea *edptr){
  if (edptr){
    ustl::string s(merge_area(edptr->elements));
    copy_clipboard(s);
    s="\n";
    edptr->elements.clear();
    edptr->clipline=-1;
    edptr->filename="\\\\fls0\\"+remove_path(giac::remove_extension(filename))+".py";
    load_script((char *)edptr->filename.c_str(),s);
    if (s.empty())
      s="\n";
    // cout << "script " << edptr->filename << endl;
    edptr->editable=true;
    edptr->changed=false;
    edptr->python=python_compat(contextptr);
    edptr->elements.clear();
    edptr->y=7;
    add(edptr,s);
    edptr->line=0;
    edptr->pos=0;
  }
}  

#if 0 // version that marks end of file with 0 0
void save_script(const char * filename,const ustl::string & s){
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  // create file in data folder (assumes data folder already exists)
  Bfile_StrToName_ncpy(pFile, filename, strlen(filename)+1);
  // even if it already exists, there's no problem,
  // in the event that our file shrinks, we just let junk be at the end of
  // the file (after two null bytes, of course).
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  if(hFile < 0) {
    // error. file does not exist yet. try creating it
    // (data folder should exist already, as save_console_state_smem() should have been called before this function)
    size_t size = 1;
    Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, &size);
    // now try opening
    hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
    if(hFile < 0) return; // if it still fails, there's nothing we can do
  }
  Bfile_WriteFile_OS(hFile, s.c_str(), s.size());
  char buf[2]={0,0};
  Bfile_WriteFile_OS(hFile, buf, 2);
  Bfile_CloseFile_OS(hFile);  
}  

#else // erase and recreate each time

void save_script(const char * filename,const ustl::string & s){
  unsigned short pFile[MAX_FILENAME_SIZE+1];
  // create file in data folder (assumes data folder already exists)
  Bfile_StrToName_ncpy(pFile, (const unsigned char *) filename, strlen(filename)+1);
  // even if it already exists, there's no problem,
  // in the event that our file shrinks, we just let junk be at the end of
  // the file (after two null bytes, of course).
  int hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
  bool open=false;
  if(hFile < 0){
  }
  else {
    size_t old=Bfile_GetFileSize_OS(hFile);
    if (old<=s.size())
      open=true;
    else {
      Bfile_CloseFile_OS(hFile);
      Bfile_DeleteEntry(pFile);
    }
  }
  if (!open){
    // file does not exist yet. try creating it
    // (data folder should exist already, as save_console_state_smem() should have been called before this function)
    size_t size = 1;
    Bfile_CreateEntry_OS(pFile, CREATEMODE_FILE, (int *)&size);
    // now try opening
    hFile = Bfile_OpenFile_OS(pFile, READWRITE); // Get handle
    if(hFile < 0) return; // if it still fails, there's nothing we can do
  }
  Bfile_WriteFile_OS(hFile, s.c_str(), s.size());
  char buf[2]={0,0};
  Bfile_WriteFile_OS(hFile, buf, 2);
  Bfile_CloseFile_OS(hFile);  
}  

#endif

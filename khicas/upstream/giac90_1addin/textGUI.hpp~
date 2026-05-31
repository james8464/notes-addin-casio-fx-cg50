#ifndef __TEXTGUI_H
#define __TEXTGUI_H
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
#include <string>
#include <vector>

void copy_clipboard(const std::string & s,bool status=false); // in dConsole.cpp

bool isalphanum(char c);
void chk_restart();

typedef struct
{
  ustl::string s;
  color_t color=COLOR_BLACK;
  short int newLine=0; // if 1, new line will be drawn before the text
  short int spaceAtEnd=0;
  short int lineSpacing=0;
  short int minimini=0;
  int nlines=1;
} textElement;

#define TEXTAREATYPE_NORMAL 0
#define TEXTAREATYPE_INSTANT_RETURN 1
typedef struct
{
  int x=0;
  int y=0;
  int line=0,undoline=0;
  int pos=0,undopos=0;
  int clipline,undoclipline;
  int clippos,undoclippos;
  int width=LCD_WIDTH_PX;
  int lineHeight=17;
  ustl::vector<textElement> elements,undoelements;
  const char* title = NULL;
  ustl::string filename;
  int scrollbar=1;
  bool allowEXE=0; //whether to allow EXE to exit the screen
  bool allowF1=0; //whether to allow F1 to exit the screen
  bool editable=false;
  bool changed=false;
  int python=0;
  int type=TEXTAREATYPE_NORMAL;
} textArea;

#define TEXTAREA_RETURN_EXIT 0
#define TEXTAREA_RETURN_EXE 1
#define TEXTAREA_RETURN_F1 2
int doTextArea(textArea* text); //returns 0 when user EXITs, 1 when allowEXE is true and user presses EXE, 2 when allowF1 is true and user presses F1.
ustl::string merge_area(const ustl::vector<textElement> & v);
void save_script(const char * filename,const ustl::string & s);
void add(textArea *edptr,const ustl::string & s);

extern textArea * edptr;
ustl::string get_searchitem(ustl::string & replace);
int check_leave(textArea * text);
void reload_edptr(const char * filename,textArea *edptr);
void print(int &X,int&Y,const char * buf,int color,bool revert,bool fake,bool minimini);

#endif 

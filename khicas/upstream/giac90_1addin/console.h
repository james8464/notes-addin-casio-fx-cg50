/*
  Console Lib for CASIO fx-9860G
  by Mike Smith.
*/

#ifndef CONSOLE_H
#define CONSOLE_H
#define INPUTBUFLEN 500
//command history numbers:
#define N 41
#define HISTORYHEAP_N N+4
// extra F keys

//#include "giacPCH.h"
#include <stdlib.h>
#include <string.h>


ustl::string help_insert(const char * cmdline,int & back,bool warn);

#define MAX_FILENAME_SIZE 270
#define DATAFOLDER "\\\\fls0\\@KHICAS"
//#define CONSOLESTATEFILE (char*)DATAFOLDER"\\khicas.erd"
void save_console_state_smem(const char * filename);
bool load_console_state_smem(const char * filename);
int inputline(const char * msg1,const char * msg2,ustl::string & s,bool numeric,int ypos=65);
bool inputdouble(const char * msg1,double & d);

struct DISPBOX {
  int     left;
  int     top;
  int     right;
  int     bottom;
  unsigned char mode;
} ;

enum CONSOLE_SCREEN_SPEC{
#ifdef CURSOR
  LINE_MAX = 32,
  LINE_DISP_MAX = 7,
  COL_DISP_MAX = 21,//21
#else
  LINE_MAX = 48,
  LINE_DISP_MAX = 9,
  COL_DISP_MAX = 32,//21
#endif
  EDIT_LINE_MAX = 2048
};

enum CONSOLE_RETURN_VAL{
  CONSOLE_NEW_LINE_SET = 1,
  CONSOLE_SUCCEEDED = 0,
  CONSOLE_MEM_ERR = -1,
  CONSOLE_ARG_ERR = -2,
  CONSOLE_NO_EVENT = -3
};

enum CONSOLE_CURSOR_DIRECTION{
  CURSOR_UP,
  CURSOR_DOWN,
  CURSOR_LEFT,
  CURSOR_RIGHT,
  CURSOR_SHIFT_LEFT,
  CURSOR_SHIFT_RIGHT,
  CURSOR_ALPHA_UP,
  CURSOR_ALPHA_DOWN,
};

enum CONSOLE_LINE_TYPE{
  LINE_TYPE_INPUT=0,
  LINE_TYPE_OUTPUT=1
};

enum CONSOLE_CASE{
  LOWER_CASE,
  UPPER_CASE
};

struct console_line {
  unsigned char *str;
  short int readonly;
  short int type;
  int start_col;
  int disp_len;
};

struct FMenu{
  char* name;
  char** str;
  unsigned char count;
};

struct location{
  int x;
  int y;
};

#define MAX_FMENU_ITEMS 8
#define FMENU_TITLE_LENGHT 4

#define is_wchar(c) ((c == 0x7F) || (c == 0xF7) || (c == 0xF9) || (c == 0xE5) || (c == 0xE6) || (c == 0xE7))
#define printf(s) Console_Output((const unsigned char *)s);

giac::gen select_var();
int Console_DelStr(unsigned char *str, int end_pos, int n);
int Console_InsStr(unsigned char *dest, const unsigned char *src, int disp_pos);
int Console_GetActualPos(const unsigned char *str, int disp_pos);
int Console_GetDispLen(const unsigned char *str);
int Console_MoveCursor(int direction);
int Console_Input(const unsigned char *str);
int Console_Output(const unsigned char *str);
void Console_Clear_EditLine();
int Console_NewLine(int pre_line_type, int pre_line_readonly);
int Console_Backspace(void);
int Console_GetKey(void);
int Console_Init(void);
int Console_Disp(void);
int Console_FMenu(int key);
extern char menu_f1[],menu_f2[],menu_f3[],menu_f4[],menu_f5[],menu_f6[],menu_f7[],menu_f8[],menu_f9[],menu_f10[],menu_f11[],menu_f12[],menu_f13[],menu_f14[],menu_f15[],menu_f16[],menu_f17[],menu_f18[];
const char * console_menu(int key,unsigned char* cfg,int active_app);
const char * console_menu(int key,int active_app);
void Console_FMenu_Init(void);
const char * Console_Draw_FMenu(int key, struct FMenu* menu,unsigned char * cfg_,int active_app);
char *Console_Make_Entry(const unsigned char* str);
unsigned char *Console_GetLine(void);
unsigned char* Console_GetEditLine();
void dConsolePut(const char *);
void dConsolePutChar(const char );
void dConsoleRedraw(void);
extern int dconsole_mode;
extern int console_changed; // 1 if something new in history
extern char session_filename[MAX_FILENAME_SIZE+1];
void translate_fkey(int & input_key);
void delete_clipboard();
const char * paste_clipboard();
const char * input_matrix(bool list);
extern char xcas_status[64];
void set_xcas_status();

extern int xthetat;
const char * keytostring(int key,int keyflag); // key to char *, 0 if not handled
const char * keytostring(int key,int keyflag,bool py,const giac::context * contextptr);
int handle_f5();
int get_filename(char * filename,const char * extension);

int print_msg12(const char * msg1,const char * msg2,int textY=40);
bool do_confirm(const char * s);
int confirm(const char * msg1,const char * msg2,bool acexit=false);
bool confirm_overwrite();
void invalid_varname();

void warn_python(int python,bool autochange=false);
// void draw_menu(int editor); // 0 console, 1 editor
int get_set_session_setting(int value);
int get_line_number(const char * msg1,const char * msg2);
void PrintMini(int x,int y,const char * s,int mode);
void menu_setup();
#endif

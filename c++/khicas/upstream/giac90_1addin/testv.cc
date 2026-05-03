// -*- compile-command: "sh3eb-elf-g++ -g -mb -m4a-nofpu  -mhitachi -std=c++11 -fpermissive -fno-use-cxa-atexit -fno-strict-aliasing -fno-rtti -fno-exceptions  -I. -I/home/parisse/casiolocal/include/ustl  -c r8c2.cc -o r8c2.o && sh3eb-elf-g++ -g -mb -m4a-nofpu  -mhitachi -std=c++11 -fpermissive -fno-use-cxa-atexit -fno-strict-aliasing -fno-rtti -fno-exceptions  -I. -I/home/parisse/casiolocal/include/ustl  -c testv.cc -o testv.o && sh3eb-elf-g++ testv.o r8c2.o -g -static -nostdlib -Taddinram.ld -Wl,--gc-sections,--print-memory-usage -L. -L/home/parisse/casiolocal/lib -Wl,--start-group -ltommath -lustl -lm -lc -lgcc -Wl,--end-group -o testvcal.elf && sh3eb-elf-objdump -C -t testvcal.elf | sort > dumpcalc && sh3eb-elf-objcopy -R .comment -R .bss -R .rominram -O binary testvcal.elf testvcal.bin && sh3eb-elf-objcopy -j .rominram  -O binary testvcal.elf testv.ac2 && sh3eb-elf-g++ testv.o r8c2.o -g -static -nostdlib -Taddinemuram.ld -Wl,--gc-sections,--print-memory-usage -L. -L/home/parisse/casiolocal/lib -Wl,--start-group -ltommath -lustl -lm -lc -lgcc -Wl,--end-group -o testvemu.elf && sh3eb-elf-objcopy -R .comment -R .bss -R .rominram -O binary testvemu.elf testvemu.bin && sh3eb-elf-objdump -C -t testvemu.elf | sort > dumpemu &&  sh3eb-elf-objcopy -j .rominram  -O binary testvemu.elf testv.882 && mkg3a -n basic:Testv -n internal:TESTV -V 0.0.0 testvcal.bin testvcal.g3a && mkg3a -n basic:Testv -n internal:TESTV -V 0.0.0 testvemu.bin testvemu.g3a && /bin/cp testvemu.g3a testv.882 ~/.wine/drive_c && /bin/cp testvcal.g3a testv.ac2 /shared/PrizmSDK-0.3"  -*-
// debug with sh3eb-elf-gdb -i=mi -ex "target remote localhost:31188" testv.elf

#include <fxcg/keyboard.h>
#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define std ustl
#include <vector>
#include <string>
#include "r8c2.h"
using namespace ustl;

unsigned ram2Msize=0;
const char ram_filename[]="\\\\fls0\\testv.8c2";

int getkey(){
  unsigned char buffer[12];
  PRGM_GetKey_OS( buffer );
  int res=( buffer[1] & 0x0F ) * 10 + ( ( buffer[2] & 0xF0 ) >> 4 );
    return res;
}

bool shiftstate=false,alphastate=false,alphalock=false,alphamaj=true;

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
  KEY_CTRL_SHIFT,KEY_CTRL_OPTN,KEY_CTRL_VARS,9,KEY_CTRL_LEFT,KEY_CTRL_UP,
  KEY_CTRL_ALPHA,KEY_CHAR_SQUARE,KEY_CHAR_POW,KEY_CTRL_EXIT,KEY_CTRL_DOWN,KEY_CTRL_RIGHT,
  KEY_CTRL_XTT,KEY_CHAR_LOG,KEY_CHAR_LN,KEY_CHAR_SIN,KEY_CHAR_COS,KEY_CHAR_TAN,
  KEY_CHAR_FRAC,KEY_CTRL_F14,KEY_CHAR_LPAR,KEY_CHAR_RPAR,KEY_CHAR_COMMA,KEY_CHAR_STORE,
  '7','8','9',KEY_CTRL_DEL,KEY_CTRL_AC,65535,
  '4','5','6','*','/',65535,
  '1','2','3','+','-',65535,
  '0','.',KEY_CHAR_EXPN10,KEY_CHAR_PMINUS,KEY_CTRL_EXE,65535,
  // shifted
  KEY_CTRL_F7,KEY_CTRL_F8,KEY_CTRL_F9,KEY_CTRL_F10,KEY_CTRL_F11,KEY_CTRL_F12,
  KEY_CTRL_SHIFT,KEY_CTRL_F13,KEY_CTRL_PRGM,9,10,11,
  KEY_CTRL_ALPHA,KEY_CHAR_ROOT,KEY_CHAR_POWROOT,KEY_CTRL_QUIT,KEY_SHIFT_LEFT,KEY_CTRL_PAGEUP,
  KEY_CHAR_ANGLE,KEY_CHAR_EXPN10,KEY_CHAR_EXP,KEY_CHAR_ASIN,KEY_CHAR_ACOS,KEY_CHAR_ATAN,
  KEY_CTRL_MIXEDFRAC,KEY_CTRL_FRACCNVRT,KEY_CHAR_CUBEROOT,KEY_CHAR_RECIP,KEY_SAVE,KEY_LOAD,
  KEY_CTRL_CAPTURE,KEY_CTRL_CLIP,KEY_CTRL_PASTE,KEY_CTRL_INS,KEY_PRGM_ACON,65535,
  KEY_CTRL_CATALOG,KEY_CTRL_FORMAT,KEY_CTRL_F6,KEY_CHAR_LBRACE,KEY_CHAR_RBRACE,65535,
  KEY_CHAR_LIST,KEY_CHAR_MAT,KEY_CTRL_F3,KEY_CHAR_LBRCKT,KEY_CHAR_RBRCKT,65535,
  KEY_CHAR_IMGNRY,KEY_CHAR_EQUAL,KEY_CHAR_PI,KEY_CHAR_ANS,KEY_PRGM_RETURN,65535,
  // alpha
  KEY_CTRL_F1,KEY_CTRL_F2,KEY_CTRL_F3,KEY_CTRL_F4,KEY_CTRL_F5,KEY_CTRL_F6,
  KEY_CTRL_SHIFT,KEY_CTRL_OPTN,KEY_CTRL_VARS,9,KEY_CTRL_LEFT,KEY_CTRL_UP,
  KEY_CTRL_ALPHA,KEY_CHAR_VALR,KEY_CHAR_THETA,KEY_CTRL_EXIT,KEY_CTRL_DOWN,KEY_CTRL_RIGHT,
  'A','B','C','D','E','F',
  'G','H','I','J','K','L',
  'M','N','O',KEY_CTRL_UNDO,KEY_CTRL_AC,65535,
  'P','Q','R','S','T',65535,
  'U','V','W','X','Y',65535,
  'Z',' ','"',KEY_CHAR_ANS,KEY_CTRL_EXE,65535,
  // alpha shifted
  KEY_CTRL_F1,KEY_CTRL_F2,KEY_CTRL_F3,KEY_CTRL_F4,KEY_CTRL_F5,KEY_CTRL_F6,
  KEY_CTRL_SHIFT,KEY_CTRL_OPTN,KEY_CTRL_VARS,9,KEY_CTRL_LEFT,KEY_CTRL_UP,
  KEY_CTRL_ALPHA,KEY_CHAR_VALR,KEY_CHAR_THETA,KEY_CTRL_EXIT,KEY_CTRL_DOWN,KEY_CTRL_RIGHT,
  'a','b','c','d','e','f',
  'g','h','i','j','k','l',
  'm','n','o',KEY_CTRL_UNDO,KEY_CTRL_AC,65535,
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
    OS_InnerWait_ms(50);
    // menu row 9 col 4
    // Keyboard_PutKeycode(4,9,KEY_CTRL_MENU);
    Keyboard_PutKeycode(4,9,0);
  }
}

void set_menu_timer(){
  if (menutimer) return;
  menutimer = Timer_Install(0,send_menu_key, 200);
  if (menutimer > 0) {
    Timer_Start(menutimer);
  }
}
int ck_getkey(int * keyptr){
  int col, row;
  unsigned short keycode=0;
  shiftstate=false;alphastate=false;alphalock=false;alphamaj=true;
  int keyflag=GetSetupSetting( (unsigned int)0x14);
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
    int ret=GetKeyWait_OS(&col,&row, 2 /* KEYWAIT_HALTON_TIMERON*/, 30 /*timeout_period*/, 0 /* handle menu key*/, &keycode) ;
    if (ret==2 /* timeout */ ){
      //exit(1);
#if 0
      printf("Pour relancer, taper MENU COS MENU");	
      Bdisp_PutDisp_DD ();
      PowerOff(1);
      for (;;){
	GetKey(keyptr);
      }
#endif
      // code=35; shiftstate=true;
      printf("timeout menu \n");
      Bdisp_PutDisp_DD ();
      set_menu_timer();
      GetKey(keyptr);
      *keyptr=KEY_SHUTDOWN;
      return 1;
      // PowerOff(1);
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
    char buf[256],buf2[256];
    sprint_int(buf,row);
    sprint_int(buf2,col);
    printf("row %s col %s\n",buf,buf2);
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
      printf("off menu \n");
      shiftstate=false;
      // PowerOff(1);
      set_menu_timer();
      GetKey(keyptr);
      casiostatus();
      continue;
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
    shiftstate=false;
    if (!alphalock)
      alphastate=false;
    casiostatus();
    return 1;
  }
}

bool chk_ram2M(const char * filename,bool reload){
  bool is_emulator = *(volatile uint32_t *)0xff000044 == 0x00000000;
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
      return true; // proba to return true with corruption is tiny, about 1/4e9
    if (!reload)
      return false;
  }
  // load ram part, filename="khicas.882" or "khicas.ac2"
  int nchar=strlen(filename);
  if (nchar<5 || filename[nchar-4]!='.' || filename[nchar-1]!='2')
    return false;
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
    return false; // nothing to load
  int size=Bfile_GetFileSize_OS(hf);
  if (size>2*1024*1024-4 || Bfile_ReadFile_OS(hf,ram2M,size,0)!=size){
    Bfile_CloseFile_OS(hf);
    return false;
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
  return true;
}

int main(){
  if (!chk_ram2M(ram_filename,true)){
    printf("%s%s\n","Unable to load ",ram_filename);
    int key;
    for(;;)
      GetKey(&key);
    return 0;
  }
  if (!chk_ram2M(ram_filename,false)){
    printf("%s\n"," ",ram_filename);
    int key;
    for(;;)
      GetKey(&key);
    return 0;
  }
  int i=4321;
  string s("Coucou");
  //int * jptr=new int;
  char * buf=(char *)malloc(256);
  char * buf2=new char[256];
  while (1){
    printf("Type a key\n");
    ck_getkey(&i);
    if (i==KEY_SHUTDOWN) // KEY_PRGM_ACON || i==KEY_PRGM_EXIT)
      break;
    sprint_int(buf,i);
    printf("%s\n",buf);
  }
  // printf("malloc %d\n new %d\nstack %d",unsigned(buf)-0x88100000,unsigned(buf2)-0x8100000,unsigned(&buf)-0x88100000);
  strcpy(buf,"!!Hello abcdefghijklmnopqrstuvwxyz 123456789");
  strcpy(buf2,"!!Hello2 abcdefghijklmnopqrstuvwxyz 123456789");
  printf("%s\n",buf);
  printf("%s\n",buf2);
  // char buf[]="!!Hello abcdefghijklmnopqrstuvwxyz 123456789";
  if (0){ 
    std::vector<int> v(3);
    for (int j=0;j<v.size();++j){
      v[j]=j*j;
    }
    r8c2_print_vector(v);
    i=unsigned (&v[2])-0x08100000;
    // v.clear();
  }
  //i=unsigned(buf)-0x08100000;
  int * Ptr=new int;
  delete Ptr;
  int *pt=new int[10];
  int k=unsigned(buf)-0x08100000;
  //for (int j=0;j<v.size();++j) v[j]=j*j;
  //sprint_int(buf,123456);
  double d=123456.78; 
  char tmp[]="12g";
  char * ptr;
  //int j=strtol(tmp,&ptr,10);
  //d=ptr-tmp;
  //sprint_double(buf+2,d);
  //sprint_int(buf2+2,i);
  //sprintf(buf+2,"Co %d %.10f %s\n",k,d,"XXX");
  //sprintf(buf2+2,"Co %d %.10f %s",i,d,"XXX");
  //sprintf(buf2+2,"Co %d %.10f %s\n",i,d,(s+s).c_str());
  free(buf);
  //Bdisp_EnableColor(0);//Use 3-bit mode
  //Bdisp_AllClr_VRAM();
  //PrintXY(1,1,buf,TEXT_MODE_TRANSPARENT_BACKGROUND,TEXT_COLOR_BLACK);
  //PrintXY(1,2,buf2,TEXT_MODE_TRANSPARENT_BACKGROUND,TEXT_COLOR_BLACK);
  //OS_InnerWait_ms(2000);
  int key;
  for(;;)
    GetKey(&key);
  return 0;
}

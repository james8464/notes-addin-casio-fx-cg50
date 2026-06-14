#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


CATEGORIES = {
    "CAT_CATEGORY_ALL": 0,
    "CAT_CATEGORY_MECH": 1,
    "CAT_CATEGORY_STATS": 2,
    "CAT_CATEGORY_DATA": 3,
    "CAT_CATEGORY_NUMBER": 4,
    "CAT_CATEGORY_FLOAT": 5,
    "CAT_CATEGORY_STORAGE": 6,
    "CAT_CATEGORY_ALGO": 7,
    "CAT_CATEGORY_BOOL": 8,
}

P3_COMMANDS = [
    ("suvat(u=2,a=3,t=4)", "CAT_CATEGORY_MECH"),
    ("suvat(", "CAT_CATEGORY_MECH"),
    ("projectile(20,30)", "CAT_CATEGORY_MECH"),
    ("projectile(", "CAT_CATEGORY_MECH"),
    ("projectileh(", "CAT_CATEGORY_MECH"),
    ("projectiley(", "CAT_CATEGORY_MECH"),
    ("projectileat(", "CAT_CATEGORY_MECH"),
    ("projectileangle(", "CAT_CATEGORY_MECH"),
    ("force(12,3)", "CAT_CATEGORY_MECH"),
    ("force(", "CAT_CATEGORY_MECH"),
    ("weight(", "CAT_CATEGORY_MECH"),
    ("friction(", "CAT_CATEGORY_MECH"),
    ("incline(5,30,0.2)", "CAT_CATEGORY_MECH"),
    ("incline(", "CAT_CATEGORY_MECH"),
    ("inclineacc(", "CAT_CATEGORY_MECH"),
    ("pulley(3,5)", "CAT_CATEGORY_MECH"),
    ("pulley(", "CAT_CATEGORY_MECH"),
    ("connected(", "CAT_CATEGORY_MECH"),
    ("beam(10,30,4,20)", "CAT_CATEGORY_MECH"),
    ("beam(", "CAT_CATEGORY_MECH"),
    ("moment(", "CAT_CATEGORY_MECH"),
    ("ladder(", "CAT_CATEGORY_MECH"),
    ("varacct(6,-4,0,3,5)", "CAT_CATEGORY_MECH"),
    ("varacct(", "CAT_CATEGORY_MECH"),
    ("varaccx(", "CAT_CATEGORY_MECH"),
    ("hypbinom(20,0.4,4,0.05,-1)", "CAT_CATEGORY_STATS"),
    ("hypbinom(", "CAT_CATEGORY_STATS"),
    ("critbinom(", "CAT_CATEGORY_STATS"),
    ("binomcrit(", "CAT_CATEGORY_STATS"),
    ("normalprob(40,60,50,10)", "CAT_CATEGORY_STATS"),
    ("normalprob(", "CAT_CATEGORY_STATS"),
    ("normalcrit(", "CAT_CATEGORY_STATS"),
    ("samplemean(", "CAT_CATEGORY_STATS"),
    ("samplemeantail(", "CAT_CATEGORY_STATS"),
    ("binom(10,0.4,3)", "CAT_CATEGORY_STATS"),
    ("binom(", "CAT_CATEGORY_STATS"),
    ("binomtail(", "CAT_CATEGORY_STATS"),
    ("binomnorm(", "CAT_CATEGORY_STATS"),
    ("largebinomnormal(", "CAT_CATEGORY_STATS"),
    ("poisson(", "CAT_CATEGORY_STATS"),
    ("poissontail(", "CAT_CATEGORY_STATS"),
    ("cond(0.2,0.5)", "CAT_CATEGORY_DATA"),
    ("cond(", "CAT_CATEGORY_DATA"),
    ("probor(", "CAT_CATEGORY_DATA"),
    ("regresscalc(5,20,30,10,18,8)", "CAT_CATEGORY_DATA"),
    ("regresscalc(", "CAT_CATEGORY_DATA"),
    ("groupmean(5,12,15,30,25,18)", "CAT_CATEGORY_DATA"),
    ("groupmean(", "CAT_CATEGORY_DATA"),
    ("histdensity(", "CAT_CATEGORY_DATA"),
]

CS_COMMANDS = [
    ("convert(45,10,16)", "CAT_CATEGORY_NUMBER"),
    ("convert(", "CAT_CATEGORY_NUMBER"),
    ("base(", "CAT_CATEGORY_NUMBER"),
    ("bin(", "CAT_CATEGORY_NUMBER"),
    ("hex(", "CAT_CATEGORY_NUMBER"),
    ("den(", "CAT_CATEGORY_NUMBER"),
    ("bitsneeded(127)", "CAT_CATEGORY_NUMBER"),
    ("bitsneeded(", "CAT_CATEGORY_NUMBER"),
    ("unsignedrange(", "CAT_CATEGORY_NUMBER"),
    ("twosrange(", "CAT_CATEGORY_NUMBER"),
    ("signmagrange(", "CAT_CATEGORY_NUMBER"),
    ("onesrange(", "CAT_CATEGORY_NUMBER"),
    ("twos(-5,8)", "CAT_CATEGORY_NUMBER"),
    ("twos(", "CAT_CATEGORY_NUMBER"),
    ("twosdec(", "CAT_CATEGORY_NUMBER"),
    ("twosadd(", "CAT_CATEGORY_NUMBER"),
    ("twossub(5,9,8)", "CAT_CATEGORY_NUMBER"),
    ("twossub(", "CAT_CATEGORY_NUMBER"),
    ("signmag(", "CAT_CATEGORY_NUMBER"),
    ("signmagdec(", "CAT_CATEGORY_NUMBER"),
    ("ones(", "CAT_CATEGORY_NUMBER"),
    ("onesdec(", "CAT_CATEGORY_NUMBER"),
    ("binadd(1011,0110,4)", "CAT_CATEGORY_NUMBER"),
    ("binadd(", "CAT_CATEGORY_NUMBER"),
    ("binsub(", "CAT_CATEGORY_NUMBER"),
    ("shift(", "CAT_CATEGORY_NUMBER"),
    ("arithshift(", "CAT_CATEGORY_NUMBER"),
    ("fixed(", "CAT_CATEGORY_FLOAT"),
    ("fixedtc(", "CAT_CATEGORY_FLOAT"),
    ("fixedenc(5.625,3,3)", "CAT_CATEGORY_FLOAT"),
    ("fixedenc(", "CAT_CATEGORY_FLOAT"),
    ("fixedfrac(", "CAT_CATEGORY_FLOAT"),
    ("fixedtcenc(", "CAT_CATEGORY_FLOAT"),
    ("floatdec(0101100,11101)", "CAT_CATEGORY_FLOAT"),
    ("floatdec(", "CAT_CATEGORY_FLOAT"),
    ("floatenc(12.75,8,4)", "CAT_CATEGORY_FLOAT"),
    ("floatenc(", "CAT_CATEGORY_FLOAT"),
    ("floatnorm(00011010,0110)", "CAT_CATEGORY_FLOAT"),
    ("floatnorm(", "CAT_CATEGORY_FLOAT"),
    ("floatrange(8,4)", "CAT_CATEGORY_FLOAT"),
    ("floatrange(", "CAT_CATEGORY_FLOAT"),
    ("floatnearest(", "CAT_CATEGORY_FLOAT"),
    ("image(800,600,24)", "CAT_CATEGORY_STORAGE"),
    ("image(", "CAT_CATEGORY_STORAGE"),
    ("imagesize(", "CAT_CATEGORY_STORAGE"),
    ("sound(44100,60,16,2)", "CAT_CATEGORY_STORAGE"),
    ("sound(", "CAT_CATEGORY_STORAGE"),
    ("soundsize(", "CAT_CATEGORY_STORAGE"),
    ("bitrate(48000000,12)", "CAT_CATEGORY_STORAGE"),
    ("bitrate(", "CAT_CATEGORY_STORAGE"),
    ("compress(1000,250)", "CAT_CATEGORY_STORAGE"),
    ("compress(", "CAT_CATEGORY_STORAGE"),
    ("chars(120,8)", "CAT_CATEGORY_STORAGE"),
    ("chars(", "CAT_CATEGORY_STORAGE"),
    ("records(1200,32)", "CAT_CATEGORY_STORAGE"),
    ("records(", "CAT_CATEGORY_STORAGE"),
    ("ascii(", "CAT_CATEGORY_STORAGE"),
    ("binarysearch(7,1,3,5,7,9)", "CAT_CATEGORY_ALGO"),
    ("binarysearch(", "CAT_CATEGORY_ALGO"),
    ("linearsearch(", "CAT_CATEGORY_ALGO"),
    ("bubblesort(", "CAT_CATEGORY_ALGO"),
    ("insertionsort(", "CAT_CATEGORY_ALGO"),
    ("selectionsort(", "CAT_CATEGORY_ALGO"),
    ("mergesort(", "CAT_CATEGORY_ALGO"),
    ("stack(", "CAT_CATEGORY_ALGO"),
    ("queue(", "CAT_CATEGORY_ALGO"),
    ("preorder(", "CAT_CATEGORY_ALGO"),
    ("inorder(", "CAT_CATEGORY_ALGO"),
    ("postorder(", "CAT_CATEGORY_ALGO"),
    ("dijkstra(", "CAT_CATEGORY_ALGO"),
    ("bool(", "CAT_CATEGORY_BOOL"),
    ("truth(", "CAT_CATEGORY_BOOL"),
    ("demorgan(", "CAT_CATEGORY_BOOL"),
    ("nand(", "CAT_CATEGORY_BOOL"),
]

P3_FOLDERS = [
    ("Mechanics", "CAT_CATEGORY_MECH"),
    ("Statistics", "CAT_CATEGORY_STATS"),
    ("Data/prob", "CAT_CATEGORY_DATA"),
]

CS_FOLDERS = [
    ("Number", "CAT_CATEGORY_NUMBER"),
    ("Float/fixed", "CAT_CATEGORY_FLOAT"),
    ("Storage", "CAT_CATEGORY_STORAGE"),
    ("Algorithms", "CAT_CATEGORY_ALGO"),
    ("Boolean", "CAT_CATEGORY_BOOL"),
]


def emit(app: str) -> str:
    if app == "p3":
        commands, folders, title, about = P3_COMMANDS, P3_FOLDERS, "CASP3 Catalog", "CASP3"
    elif app == "cs":
        commands, folders, title, about = CS_COMMANDS, CS_FOLDERS, "CSCalc Catalog", "CSCalc"
    else:
        raise SystemExit("usage: khicas_suite_catalog.py p3|cs")

    cat_defs = "\n".join(f"#define {name} {value}" for name, value in CATEGORIES.items())
    command_rows = "\n".join(f'  {{(char*)"{name}", {cat}}},' for name, cat in commands)
    folder_rows = "\n".join(f'  {{(char*)"{name}", {cat}}},' for name, cat in folders)
    return f'''#include <stdio.h>
#include <fxcg/keyboard.h>
#include "giacPCH.h"
#include <fxcg/display.h>
#include <fxcg/file.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>
#include <fxcg/misc.h>
#include <fxcg/app.h>
#include <fxcg/serial.h>
extern "C" {{
#include <fxcg/rtc.h>
}}
#include <fxcg/heap.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>

#include "history.h"
#include "dConsole.h"
#define CASCAS_MIN_CATALOG
#include "catalogGUI.hpp"
#include "fileGUI.hpp"
#include "graphicsProvider.hpp"
#include "constantsProvider.hpp"
#include "kdisplay.h"
#include "input_lexer.h"
#include "input_parser.h"
#include "graphicsProvider.hpp"
#define STATUS_AREA_PX 24

{cat_defs}

using namespace giac;
extern giac::context * contextptr;
void reset_alpha(){{
  SetSetupSetting( (unsigned int)0x14, 0);
  DisplayStatusArea();
}}

int lang=0;
const catalogFunc completeCat[] = {{
{command_rows}
}};

typedef struct {{
  char* title;
  int category;
}} catalogFolder;

const catalogFolder catalogFolders[] = {{
{folder_rows}
}};

const char chk_restart_string1[]="Keep variables?";
const char chk_restart_string2[]="F1: keep,   F6: erase";
const char aide_khicas_string[]="{title}";
const char main_string1[]="Clear variables?";
const char main_string2[]="F1: cancel,  F6: confirm";
const char shortcuts_string[]="Use catalog for commands.";
const char apropos_string[]="{about}";

int CAT_COMPLETE_COUNT=sizeof(completeCat)/sizeof(catalogFunc);
int CAT_FOLDER_COUNT=sizeof(catalogFolders)/sizeof(catalogFolder);

ustl::string insert_string(int index){{
  ustl::string s=completeCat[index].name;
  int pos=s.find('(');
  if (pos>=0 && pos<s.size())
    s=s.substr(0,pos+1);
  return s;
}}

int showCatalog(char* insertText,int preselect,int menupos) {{
  return doCatalogMenu(insertText, (char*)"{title}", CAT_CATEGORY_ALL);
}}

static int catalog_count_for_category(int category){{
  if (category==CAT_CATEGORY_ALL)
    return CAT_FOLDER_COUNT;
  int n=0;
  for (int i=0;i<CAT_COMPLETE_COUNT;++i)
    if (completeCat[i].category==category)
      ++n;
  return n;
}}

static void draw_catalog_fkeys(bool folders){{
  drawFkeyLabels(folders?0:0x03FC,0,0,0,0,folders?0:0x03FD);
}}

int doCatalogMenu(char* insertText, char* title, int category,const char * cmdname) {{
  int allcmds=CAT_COMPLETE_COUNT;
  int nitems = catalog_count_for_category(category);
  if (nitems<=0)
    return 0;
  MenuItem* menuitems = (MenuItem*)alloca(sizeof(MenuItem)*nitems);
  int cur = 0,curmi = 0,menusel=-1;
  if (category==CAT_CATEGORY_ALL){{
    while(cur<CAT_FOLDER_COUNT) {{
      menuitems[curmi].type = MENUITEM_NORMAL;
      menuitems[curmi].color = TEXT_COLOR_BLUE;
      menuitems[curmi].isfolder = cur+1;
      menuitems[curmi].text = catalogFolders[cur].title;
      curmi++;
      cur++;
    }}
  }}
  else {{
    while(cur<allcmds) {{
      if (completeCat[cur].category==category){{
        menuitems[curmi].type = MENUITEM_NORMAL;
        menuitems[curmi].color = TEXT_COLOR_BLACK;
        menuitems[curmi].isfolder = cur;
        menuitems[curmi].text = completeCat[cur].name;
        curmi++;
      }}
      cur++;
    }}
  }}

  Menu menu;
  if (menusel>=0)
    menu.selection=menusel+1;
  menu.items=menuitems;
  menu.numitems=curmi;
  if (curmi>=100)
    SetSetupSetting( (unsigned int)0x14, 0x88);
  DisplayStatusArea();
  menu.scrollout=1;
  menu.title = title;
  menu.type = category==CAT_CATEGORY_ALL ? MENUTYPE_NORMAL : MENUTYPE_FKEYS;
  menu.height = 7;
  while(1) {{
    draw_catalog_fkeys(category==CAT_CATEGORY_ALL);
    int sres = doMenu(&menu);
    if(sres == MENU_RETURN_EXIT){{
      reset_alpha();
      return 0;
    }}
    int index=menuitems[menu.selection-1].isfolder;
    if(category!=CAT_CATEGORY_ALL && sres == KEY_CTRL_F6) {{
      sres=KEY_CTRL_F1;
    }}
    if(sres == MENU_RETURN_SELECTION || sres == KEY_CTRL_F1) {{
      if (category==CAT_CATEGORY_ALL){{
        int folder=index-1;
        if (folder>=0 && folder<CAT_FOLDER_COUNT){{
          if (doCatalogMenu(insertText,catalogFolders[folder].title,catalogFolders[folder].category))
            return 1;
          DisplayStatusArea();
        }}
        continue;
      }}
      reset_alpha();
      strcpy(insertText,index<allcmds?insert_string(index).c_str():menuitems[menu.selection-1].text);
      return 1;
    }}
  }}
  return 0;
}}
'''


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: khicas_suite_catalog.py p3|cs output")
    Path(sys.argv[2]).write_text(emit(sys.argv[1]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


CATEGORIES = {
    "CAT_CATEGORY_ALL": 0,
    "CAT_CATEGORY_MECH": 1,
    "CAT_CATEGORY_STATS": 2,
    "CAT_CATEGORY_DATA": 3,
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
    ("vectorkin(", "CAT_CATEGORY_MECH"),
    ("hypbinom(20,0.4,4,0.05,-1)", "CAT_CATEGORY_STATS"),
    ("hypbinom(", "CAT_CATEGORY_STATS"),
    ("critbinom(", "CAT_CATEGORY_STATS"),
    ("binomcrit(", "CAT_CATEGORY_STATS"),
    ("normalprob(40,60,50,10)", "CAT_CATEGORY_STATS"),
    ("normalprob(", "CAT_CATEGORY_STATS"),
    ("normalprobvar(", "CAT_CATEGORY_STATS"),
    ("normaltail(", "CAT_CATEGORY_STATS"),
    ("invnormal(", "CAT_CATEGORY_STATS"),
    ("normalcrit(", "CAT_CATEGORY_STATS"),
    ("normalparams(", "CAT_CATEGORY_STATS"),
    ("samplemean(", "CAT_CATEGORY_STATS"),
    ("samplemeantail(", "CAT_CATEGORY_STATS"),
    ("binom(10,0.4,3)", "CAT_CATEGORY_STATS"),
    ("binom(", "CAT_CATEGORY_STATS"),
    ("binomtail(", "CAT_CATEGORY_STATS"),
    ("binomstats(", "CAT_CATEGORY_STATS"),
    ("binomnorm(", "CAT_CATEGORY_STATS"),
    ("largebinomnormal(", "CAT_CATEGORY_STATS"),
    ("poisson(", "CAT_CATEGORY_STATS"),
    ("poissontail(", "CAT_CATEGORY_STATS"),
    ("poissonstats(", "CAT_CATEGORY_STATS"),
    ("critpoisson(", "CAT_CATEGORY_STATS"),
    ("cond(0.2,0.5)", "CAT_CATEGORY_DATA"),
    ("cond(", "CAT_CATEGORY_DATA"),
    ("probor(", "CAT_CATEGORY_DATA"),
    ("bayes(", "CAT_CATEGORY_DATA"),
    ("independent(", "CAT_CATEGORY_DATA"),
    ("regresscalc(5,20,30,10,18,8)", "CAT_CATEGORY_DATA"),
    ("regresscalc(", "CAT_CATEGORY_DATA"),
    ("groupmean(5,12,15,30,25,18)", "CAT_CATEGORY_DATA"),
    ("groupmean(", "CAT_CATEGORY_DATA"),
    ("histdensity(", "CAT_CATEGORY_DATA"),
    ("groupmedian(", "CAT_CATEGORY_DATA"),
    ("meanvar(", "CAT_CATEGORY_DATA"),
]

P3_FOLDERS = [
    ("Mechanics", "CAT_CATEGORY_MECH"),
    ("Statistics", "CAT_CATEGORY_STATS"),
    ("Data/prob", "CAT_CATEGORY_DATA"),
]

def emit(app: str) -> str:
    if app == "p3":
        commands, folders, title, about = P3_COMMANDS, P3_FOLDERS, "CASP3 Catalog", "CASP3"
    else:
        raise SystemExit("usage: khicas_suite_catalog.py p3")

    cat_defs = "\n".join(f"#define {name} {value}" for name, value in CATEGORIES.items())
    def esc(s: str) -> str:
        return s.replace("\\", "\\\\").replace('"', '\\"')

    def base_name(s: str) -> str:
        return s.split("(", 1)[0]

    def insert_text(s: str) -> str:
        b = base_name(s)
        return b + "(" if "(" in s else b

    def desc(s: str) -> str:
        b = base_name(s)
        if app == "p3":
            if b in {"suvat", "varacct", "varaccx", "vectorkin", "vectorsuvat", "vectormotion"}:
                return "Mechanics working: formula, substitution, answer."
            if b.startswith("projectile"):
                return "Projectile working with components and suvat."
            if b in {"force", "weight", "friction", "incline", "inclineacc", "pulley", "connected"}:
                return "Forces working with Newton's second law."
            if b in {"beam", "moment", "ladder"}:
                return "Moments/equilibrium working."
            if b in {"hypbinom", "critbinom", "binomcrit"}:
                return "Binomial hypothesis test working and conclusion."
            if b.startswith("normal") or b.startswith("sample") or b == "invnormal":
                return "Normal distribution setup and standardisation."
            if b.startswith("binom") or b.startswith("poisson"):
                return "Discrete distribution setup and probability."
            return "Stats/data working with formula and substitution."
        return "Paper 3 command."

    def example(s: str) -> str:
        if "(" in s and ")" in s:
            return "#" + s
        examples = {
            "suvat(": "suvat(u=2,a=3,t=4)",
            "projectile(": "projectile(20,30)",
            "force(": "force(12,3)",
            "incline(": "incline(5,30,0.2)",
            "pulley(": "pulley(3,5)",
            "beam(": "beam(10,30,4,20)",
            "varacct(": "varacct(6,-4,0,3,5)",
            "hypbinom(": "hypbinom(20,0.4,4,0.05,-1)",
            "normalprob(": "normalprob(40,60,50,10)",
            "binom(": "binom(10,0.4,3)",
            "cond(": "cond(0.2,0.5)",
            "regresscalc(": "regresscalc(5,20,30,10,18,8)",
            "groupmean(": "groupmean(5,12,15,30,25,18)",
        }
        return "#" + examples.get(s, insert_text(s) + ")")

    def detail(s: str) -> tuple[str, str, str, str, str]:
        b = base_name(s)
        ex = example(s)
        if ex.startswith("#"):
            ex = ex[1:]
        if app == "p3":
            if b == "suvat":
                return ("suvat(u=...,v=...,a=...,t=...,s=...)",
                        "Give any 3 SUVAT values as named arguments.",
                        "Finds every missing constant-acceleration value.",
                        "Selects a valid SUVAT formula, substitutes, solves, then states answer lines.",
                        "suvat(u=2,a=3,t=4)")
            if b.startswith("projectile"):
                return ("projectile(speed,angle[,g]) or projectileh/s/at variants",
                        "Angle in degrees. Optional g defaults to 9.8.",
                        "Projectile components, time, range, height, or value at time.",
                        "Resolves velocity into horizontal/vertical components then applies SUVAT.",
                        "projectile(20,30), projectileat(20,30,2)")
            if b in {"force", "weight", "friction", "incline", "inclineacc", "pulley", "connected"}:
                return (f"{b}(...)",
                        "Use masses, angles, coefficients, forces or acceleration as shown by the prompt.",
                        "Mechanics force working.",
                        "Writes Newton's second law or friction/weight rule, substitutes, then solves.",
                        ex)
            if b in {"beam", "moment", "ladder"}:
                return (f"{b}(...)",
                        "Forces and distances from a pivot.",
                        "Moment/equilibrium working.",
                        "Uses clockwise moment = anticlockwise moment and resolves reactions where needed.",
                        ex)
            if b.startswith("normal") or b in {"invnormal", "samplemean", "samplemeantail"}:
                return (f"{b}(...)",
                        "Bounds/probability, mean, standard deviation, and sample size where needed.",
                        "Normal distribution setup.",
                        "Standardises with z=(x-mu)/sigma; sample mean uses sigma/sqrt(n).",
                        ex)
            if b.startswith("binom") or b in {"hypbinom", "critbinom", "binomcrit"}:
                return (f"{b}(...)",
                        "n, p, value/tail/alpha depending on the command.",
                        "Binomial probability or hypothesis-test working.",
                        "States X~B(n,p), applies nCr p^r(1-p)^(n-r), sums tails or compares alpha.",
                        ex)
            return (f"{b}(...)",
                    "Use the parameters shown in the example.",
                    desc(s),
                    "Writes formula/setup, substitutes values, then gives final answer.",
                    ex)
        return (f"{b}(...)",
                "Use the parameters shown in the example.",
                desc(s),
                "Shows setup, substitution/trace and final answer.",
                ex)

    command_rows = "\n".join(
        f'  {{(char*)"{esc(name)}", (char*)"{esc(insert_text(name))}", (char*)"{esc(desc(name))}", (char*)"{esc(example(name))}", 0, {cat}}},'
        for name, cat in commands
    )
    help_rows = "\n".join(
        f'  {{(char*)"{esc(detail(name)[0])}", (char*)"{esc(detail(name)[1])}", (char*)"{esc(detail(name)[2])}", (char*)"{esc(detail(name)[3])}", (char*)"{esc(detail(name)[4])}"}},'
        for name, _cat in commands
    )
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
#include "catalogGUI.hpp"
#include "fileGUI.hpp"
#include "textGUI.hpp"
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
  char* syntax;
  char* args;
  char* does;
  char* work;
  char* examples;
}} catalogHelp;

const catalogHelp completeHelp[] = {{
{help_rows}
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
  return completeCat[index].insert;
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
  drawFkeyLabels(folders?0:0x03FC,folders?0:0x03fc,0,0,0,folders?0:0x03FD);
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
      textArea text;
      text.editable=false;
      text.clipline=-1;
      text.title = (char*)"Command help";
      text.allowF1=true;
      text.python=python_compat(contextptr);
      ustl::vector<textElement> & elem=text.elements;
      elem = ustl::vector<textElement>(6);
      elem[0].s = completeCat[index].name;
      elem[0].color = COLOR_BLUE;
      elem[1].newLine = 1;
      elem[1].lineSpacing = 3;
      elem[1].s = ustl::string("Syntax: ") + completeHelp[index].syntax;
      elem[2].newLine = 1;
      elem[2].lineSpacing = 3;
      elem[2].s = ustl::string("Inputs: ") + completeHelp[index].args;
      elem[3].newLine = 1;
      elem[3].lineSpacing = 3;
      elem[3].s = ustl::string("Does: ") + completeHelp[index].does;
      elem[4].newLine = 1;
      elem[4].lineSpacing = 3;
      elem[4].s = ustl::string("Method: ") + completeHelp[index].work;
      elem[5].newLine = 1;
      elem[5].lineSpacing = 3;
      elem[5].s = ustl::string("Examples: ") + completeHelp[index].examples;
      sres=doTextArea(&text);
    }}
    if(category!=CAT_CATEGORY_ALL && sres == KEY_CTRL_F2) {{
      reset_alpha();
      char * example=completeCat[index].example;
      strcpy(insertText, example && example[0]=='#' ? example+1 : insert_string(index).c_str());
      return 1;
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
        raise SystemExit("usage: khicas_suite_catalog.py p3 output")
    out = Path(sys.argv[2])
    out.write_text(emit(sys.argv[1]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

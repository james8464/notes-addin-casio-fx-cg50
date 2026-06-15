#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


OLD_EVAL = """        cascas::working_string working;
        if (cascas::eval_with_working((const char *)expr,working))
          cascas_console_output(working.c_str());
        else if (cascas_reject_removed_feature((const char *)expr))
          Console_Output((const unsigned char*)\"Err: unsupported\");
        else
          run((char *)expr);
"""

NEW_EVAL = """        if (suite_eval_with_working((const char *)expr)) {
        }
        else
          Console_Output((const unsigned char*)\"Err: unsupported\");
"""

OLD_CONTEXT = """  context ct;
  contextptr=&ct;
  cascas::set_khicas_eval_callback(cascas_khicas_eval);
"""

NEW_CONTEXT = """  context ct;
  contextptr=&ct;
  cascas::set_khicas_eval_callback(cascas_khicas_eval);
  suite_register_lexer_symbols();
"""

OLD_CFG = """  original_cfg=(unsigned char *)conf_standard;
  unsigned char* cfg=original_cfg;
"""

NEW_CFG = """  const char * suite_cfg=suite_console_fmenu_config();
  original_cfg=(unsigned char *)(suite_cfg?suite_cfg:conf_standard);
  unsigned char* cfg=original_cfg;
"""

OLD_LABELS = """  {
    string menu(" ");
    menu += string(menu_f1);
    while (menu.size()<6)
      menu += " ";
    menu += "| ";
    menu += string(menu_f2);
    while (menu.size()<13)
      menu += " ";
    menu += lang?"| voir | cmds | A<>a | Fich.":"| view | cmds | A<>a | File ";
    PrintMini(0,58,menu.c_str(),4);
  }
"""

NEW_LABELS = """  {
    const char * labels[6]={menu_f1,menu_f2,menu_f3,menu_f4,menu_f5,menu_f6};
    string menu(" ");
    for (int mi=0;mi<6;++mi){
      if (mi)
        menu += "| ";
      menu += string(labels[mi]);
      while (menu.size() < unsigned(1+(mi+1)*7))
        menu += " ";
    }
    PrintMini(0,58,menu.c_str(),4);
  }
"""


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: patch_khicas_suite_main.py main.cc")
    path = Path(sys.argv[1])
    text = path.read_text()
    if '#include "khicas_suite_bridge.hpp"' not in text:
        text = text.replace(
            '#include "cascas_working.h"\n',
            '#include "cascas_working.h"\n#include "khicas_suite_bridge.hpp"\n',
            1,
        )
    if OLD_EVAL not in text:
        raise SystemExit("main.cc eval block not found")
    text = text.replace(OLD_EVAL, NEW_EVAL, 1)
    if OLD_CONTEXT not in text:
        raise SystemExit("main.cc context block not found")
    path.write_text(text.replace(OLD_CONTEXT, NEW_CONTEXT, 1))

    console = path.with_name("console.cc")
    ctext = console.read_text()
    if '#include "khicas_suite_bridge.hpp"' not in ctext:
        ctext = ctext.replace('#include "main.h"\n', '#include "main.h"\n#include "khicas_suite_bridge.hpp"\n', 1)
    if OLD_CFG not in ctext:
        raise SystemExit("console.cc fmenu config block not found")
    ctext = ctext.replace(OLD_CFG, NEW_CFG, 1)
    if OLD_LABELS not in ctext:
        raise SystemExit("console.cc fkey label block not found")
    console.write_text(ctext.replace(OLD_LABELS, NEW_LABELS, 1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

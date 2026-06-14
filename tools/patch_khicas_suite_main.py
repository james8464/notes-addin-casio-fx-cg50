#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


OLD = """        cascas::working_string working;
        if (cascas::eval_with_working((const char *)expr,working))
          cascas_console_output(working.c_str());
        else if (cascas_reject_removed_feature((const char *)expr))
          Console_Output((const unsigned char*)\"Err: unsupported\");
        else
          run((char *)expr);
"""

NEW = """        if (suite_eval_with_working((const char *)expr)) {
        }
        else
          Console_Output((const unsigned char*)\"Err: unsupported\");
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
    if OLD not in text:
        raise SystemExit("main.cc eval block not found")
    path.write_text(text.replace(OLD, NEW, 1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simulate Casio-style constraints when testing on a PC.

Mode A — MicroPython + small heap (strongest: real uPy, real GC, optional .mpy)
  micropython -X heapsize=N bootstrap.py
  If .mpy from mpy-cross 1.9.x fails to import, use --source (load src/Math/*.py).

Mode B — CPython + env CASIO_HW_SIM=1 (only enables the same in-code low-memory
  settings as the calculator; no OS memory limit).

Usage:
  python3 src/calc_files/run_device_sim.py --heap-kb 64 --source deriveProgram
  python3 src/calc_files/run_device_sim.py -i --heap-kb 48 --source intProgram
  python3 src/calc_files/run_device_sim.py --cpython -c "from src.Math.deriveProgram import solve_normal_mode, readable_show as r; print(r(solve_normal_mode('x')[2]))"
"""
from __future__ import print_function

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEFAULT_HEAP_KB = 64


def _bootstrap_mpy_mode(project: Path, module: str, from_source: bool) -> str:
    src = str((project / "src").resolve()).replace("\\", "/")
    if from_source:
        mdir = str((project / "src" / "Math").resolve()).replace("\\", "/")
        path = "sys.path[0:0] = [r'%s', r'%s']" % (mdir, src)
    else:
        cdir = str((project / "src" / "calc_files").resolve()).replace("\\", "/")
        path = "sys.path[0:0] = [r'%s', r'%s']" % (src, cdir)
    return (
        "import sys\n"
        + path
        + "\n__import__(%r)\n" % module
    )


def _run_mpy(
    mpy_bin: str,
    project: Path,
    module: str,
    heap_kb: int,
    from_source: bool,
    interactive: bool,
    timeout,
    print_cmd: bool,
) -> int:
    body = _bootstrap_mpy_mode(project, module, from_source)
    heap = max(8, int(heap_kb)) * 1024
    with tempfile.NamedTemporaryFile(
        mode="w", suffix="_casio_uPy.py", delete=False, encoding="utf-8", dir=str(project)
    ) as f:
        f.write(body)
        tpath = f.name
    cmd = [mpy_bin, "-X", "heapsize=" + str(heap)]
    if interactive:
        cmd.append("-i")
    cmd.append(tpath)
    try:
        if print_cmd:
            print(" ".join(cmd), file=sys.stderr)
        return subprocess.call(
            cmd, cwd=str(project), timeout=timeout
        )
    except OSError as e:
        print("Error: %s" % e, file=sys.stderr)
        return 127
    except subprocess.TimeoutExpired:
        print("Error: subprocess timeout", file=sys.stderr)
        return 124
    finally:
        try:
            os.unlink(tpath)
        except OSError:
            pass


def _run_cpython(
    project: Path,
    python: str,
    user_code: str,
    timeout,
    print_cmd: bool,
) -> int:
    src = str((project / "src").resolve()).replace("\\", "/")
    smath = str((project / "src" / "Math").resolve()).replace("\\", "/")
    cfiles = str((project / "src" / "calc_files").resolve()).replace("\\", "/")
    pr = str(project.resolve()).replace("\\", "/")
    text = (
        "import os, sys\n"
        "os.environ['CASIO_HW_SIM'] = '1'\n"
        "sys.path[0:0] = [r'%s', r'%s', r'%s', r'%s']\n" % (smath, src, cfiles, pr)
        + user_code
    )
    with tempfile.NamedTemporaryFile(
        mode="w", suffix="_casio_cpy.py", delete=False, encoding="utf-8", dir=str(project)
    ) as f:
        f.write(text)
        tpath = f.name
    try:
        cmd = [python, tpath]
        if print_cmd:
            print(" ".join(cmd), file=sys.stderr)
        return subprocess.call(cmd, cwd=str(project), env=os.environ, timeout=timeout)
    except subprocess.TimeoutExpired:
        print("Error: subprocess timeout", file=sys.stderr)
        return 124
    finally:
        try:
            os.unlink(tpath)
        except OSError:
            pass


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Run CASIO programs under MicroPython (heap) or CPython (CASIO_HW_SIM)."
    )
    ap.add_argument("module", nargs="?", default="deriveProgram", help="Module to import, e.g. deriveProgram")
    ap.add_argument(
        "--source", action="store_true",
        help="Load from src/Math/*.py (use if .mpy is incompatible with your micropython)",
    )
    ap.add_argument(
        "--heap-kb", type=int, default=DEFAULT_HEAP_KB,
        help="Heap size for -X heapsize= (default %d KiB)" % DEFAULT_HEAP_KB,
    )
    ap.add_argument(
        "--micropython", default=os.environ.get("MICROPY_PATH", "micropython"),
        help="micropython binary (default: MICROPY_PATH or PATH)",
    )
    ap.add_argument(
        "-i", "--interactive", action="store_true", help="micropython -i (REPL after)",
    )
    ap.add_argument(
        "--cpython", action="store_true", help="Use CPython with CASIO_HW_SIM=1 instead of micropython",
    )
    ap.add_argument(
        "--python", default=None, help="With --cpython, Python to use (default: sys.executable)",
    )
    ap.add_argument(
        "-c", type=str, default=None, metavar="CODE", dest="code",
        help="Only with --cpython: extra code appended (imports already set up)",
    )
    ap.add_argument(
        "-e", type=str, default=None, dest="file", metavar="FILE",
        help="With --cpython: read and exec file after same setup",
    )
    ap.add_argument("--root", type=Path, default=None, help="Repo root (default: parent of src/)")
    ap.add_argument("--timeout", type=int, default=None)
    ap.add_argument("--print-cmd", action="store_true", dest="print_cmd", help="Print invoked command to stderr")
    args = ap.parse_args()
    if args.cpython and args.file and args.code:
        print("Use only one of -c or -e", file=sys.stderr)
        return 2
    project = args.root or REPO
    if args.cpython:
        if args.file:
            p = project / args.file
            with open(p, "r", encoding="utf-8", errors="replace") as f:
                user = f.read()
        elif args.code:
            user = args.code + "\n"
        else:
            m = args.module
            if m.endswith(".py"):
                m = m[:-3]
            if "." in m:
                m = m.rsplit(".", 1)[-1]
            user = (
                "import importlib\n"
                "_m = importlib.import_module(%r)\n"
                "print('ok loaded', _m.__name__)\n" % m
            )
        py = args.python or sys.executable
        return _run_cpython(
            project, py, user, args.timeout, getattr(args, "print_cmd", False)
        )
    b = shutil.which(args.micropython)
    if b is None and args.micropython:
        p = os.path.expanduser(args.micropython)
        if os.path.isfile(p) and os.access(p, os.X_OK):
            b = p
    if not b:
        print(
            "Error: 'micropython' not on PATH. Install it or set --micropython / MICROPY_PATH.",
            file=sys.stderr,
        )
        return 127
    return _run_mpy(
        b,
        project,
        args.module,
        args.heap_kb,
        args.source,
        args.interactive,
        args.timeout,
        getattr(args, "print_cmd", False),
    )


if __name__ == "__main__":
    sys.exit(main() or 0)

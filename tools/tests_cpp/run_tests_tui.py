#!/usr/bin/env python3
from __future__ import annotations

"""
Run the existing python/tests/run_tests.py Textual TUI, but execute cases against
the C++ host binary (addin/host/build/casio_host) using --stdin-program mode.
"""

import os
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
HOST = REPO / "addin" / "host" / "build" / "casio_host"


def run_cpp_cli(script: str, inp: str) -> tuple[str, str]:
    if not HOST.exists():
        return ("", f"Missing host binary: {HOST}\nBuild with ./tools/build_host.sh")
    try:
        p = subprocess.run(
            [str(HOST), "--stdin-program", script],
            input=inp,
            text=True,
            capture_output=True,
            cwd=str(REPO),
            timeout=float(os.environ.get("CASIO_CPP_TIMEOUT", "20")),
        )
        return p.stdout, p.stderr
    except subprocess.TimeoutExpired as err:
        stdout = err.stdout if isinstance(err.stdout, str) else ""
        stderr = err.stderr if isinstance(err.stderr, str) else ""
        return stdout, f"{stderr}\nTimeout".strip()


def main() -> int:
    # Ensure python/tests is importable.
    sys.path.insert(0, str(REPO / "python" / "tests"))

    import run_tests as rt  # type: ignore

    # Patch the App.run_cli method to call C++ host.
    orig = rt.CasioTestApp.run_cli

    def patched_run_cli(self, script, inp):  # type: ignore[no-redef]
        return run_cpp_cli(script, inp)

    rt.CasioTestApp.run_cli = patched_run_cli  # type: ignore[attr-defined]

    # Launch the TUI app (same as python version).
    app = rt.CasioTestApp()
    app.run()
    rt.CasioTestApp.run_cli = orig  # restore
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


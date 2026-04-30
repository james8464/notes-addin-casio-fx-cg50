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


def _run(cmd: list[str], cwd: Path | None = None) -> tuple[int, str]:
    p = subprocess.run(
        cmd,
        cwd=str(cwd or REPO),
        text=True,
        capture_output=True,
    )
    out = (p.stdout or "") + (("\n" + p.stderr) if p.stderr else "")
    return p.returncode, out.strip()


def _build_host(log) -> bool:
    log("[dim]Building host (casio_host)...[/dim]")
    rc, out = _run(["./tools/build_host.sh"])
    if rc != 0:
        log(f"[bold #f87171]✗ build_host failed[/bold #f87171]\n{out}")
        return False
    log("[bold #22c55e]✓[/#22c55e] host built")
    return True


def _build_addin(log) -> bool:
    # Prefer Docker build on macOS; fall back to native fxsdk if present.
    docker_script = REPO / "tools" / "build_addin_docker.sh"
    native_script = REPO / "tools" / "build_addin.sh"

    if docker_script.exists() and (os.environ.get("CASIO_BUILD_ADDIN", "docker") == "docker"):
        log("[dim]Building add-in (.g3a) via Docker...[/dim]")
        rc, out = _run([str(docker_script)])
    else:
        log("[dim]Building add-in (.g3a) via native fxSDK...[/dim]")
        rc, out = _run([str(native_script)])

    if rc != 0:
        log(f"[bold #f87171]✗ add-in build failed[/bold #f87171]\n{out}")
        return False
    log("[bold #22c55e]✓[/#22c55e] add-in built")
    # Print resulting g3a paths (if any)
    g3as = sorted((REPO / "addin" / "build").glob("*.g3a"))
    if g3as:
        for p in g3as:
            log(f"[dim]Output:[/dim] {p}")
    else:
        log("[dim]Note:[/dim] no .g3a found under addin/build/")
    return True


def _maybe_send_to_calc(log) -> None:
    # Only works on Linux with udisks2 + fxlink. Docker-on-mac can't access USB easily.
    if os.environ.get("CASIO_SEND_CG", "0") != "1":
        log("[dim]Send skipped (set CASIO_SEND_CG=1 to attempt fxsdk send-cg on Linux).[/dim]")
        return
    if os.uname().sysname.lower() == "darwin":
        log("[dim]Send skipped on macOS (use USB storage copy).[/dim]")
        return
    if subprocess.run(["bash", "-lc", "command -v fxsdk >/dev/null 2>&1"]).returncode != 0:
        log("[dim]Send skipped (fxsdk not found).[/dim]")
        return
    log("[dim]Attempting: fxsdk send-cg[/dim]")
    rc, out = _run(["bash", "-lc", "cd addin && fxsdk send-cg"])
    if rc != 0:
        log(f"[dim]send-cg failed (non-fatal):[/dim]\n{out}")
    else:
        log("[bold #22c55e]✓[/#22c55e] sent to calculator")


def main() -> int:
    # Ensure python/tests is importable.
    sys.path.insert(0, str(REPO / "python" / "tests"))

    import run_tests as rt  # type: ignore

    # Patch the Textual App's run_cli + /compile to use C++ host + .g3a build.
    # This repo's python/tests/run_tests.py defines CASIOApp (older variants used CasioTestApp).
    AppCls = getattr(rt, "CASIOApp", None) or getattr(rt, "CasioTestApp", None)
    if AppCls is None:
        raise SystemExit("run_tests.py: couldn't find CASIOApp/CasioTestApp")

    orig = AppCls.run_cli
    orig_compile = AppCls.action_compile

    def patched_run_cli(self, script, inp):  # type: ignore[no-redef]
        return run_cpp_cli(script, inp)

    AppCls.run_cli = patched_run_cli  # type: ignore[attr-defined]

    def patched_compile(self):  # type: ignore[no-redef]
        # Mirror the Python TUI "/compile", but for C++:
        # - build host binary
        # - build add-in (.g3a)
        # - optionally send to calculator (Linux only)
        self.append_result("[bold #e07a53]▶ /compile[/bold #e07a53]")
        self.update_summary("Compiling (host + add-in)...")

        def log(msg: str) -> None:
            self.append_result(msg)

        ok = _build_host(log) and _build_addin(log)
        if ok:
            _maybe_send_to_calc(log)
            self.update_summary("Compile OK")
        else:
            self.update_summary("Compile failed")

    AppCls.action_compile = patched_compile  # type: ignore[attr-defined]

    # Launch the TUI app (same as python version).
    app = AppCls()
    app.run()
    AppCls.run_cli = orig  # restore
    AppCls.action_compile = orig_compile
    return 0


if __name__ == "__main__":
    raise SystemExit(main())


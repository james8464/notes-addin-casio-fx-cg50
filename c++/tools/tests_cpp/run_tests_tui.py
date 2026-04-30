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


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


def run_cpp_cli(script: str, inp: str) -> tuple[str, str]:
    if not HOST.exists():
        return ("", f"Missing host binary: {HOST}\nBuild with ./c++/tools/build_host.sh")
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
    log("[dim]--- Building host (casio_host) ---[/dim]")
    log("[dim]Running cmake configure...[/dim]")
    rc, out = _run(["cmake", "-S", "c++/addin/host", "-B", "c++/addin/host/build"])
    if rc != 0:
        log(f"[bold #f87171]✗ cmake configure failed[/bold #f87171]\n{out}")
        return False
    log("[dim]Compiling C++ (host)...[/dim]")
    rc, out = _run(["cmake", "--build", "c++/addin/host/build", "-j"])
    if rc != 0:
        log(f"[bold #f87171]✗ cmake build failed[/bold #f87171]\n{out}")
        return False
    log("[dim]Running smoke test...[/dim]")
    rc, out = _run(["c++/addin/host/build/device_solver_smoke"])
    if rc != 0:
        log(f"[bold #f87171]✗ smoke test failed[/bold #f87171]\n{out}")
        return False
    log("[bold #22c55e]✓[/#22c55e] host built successfully")
    log(f"[dim]Output:[/dim] c++/addin/host/build/casio_host")
    return True


def _build_addin(log) -> bool:
    log("[dim]--- Building add-in (.g3a) ---[/dim]")
    docker_script = REPO / "tools" / "build_addin_docker.sh"
    native_script = REPO / "tools" / "build_addin.sh"

    if docker_script.exists() and (os.environ.get("CASIO_BUILD_ADDIN", "docker") == "docker"):
        log("[dim]Building fxSDK Docker image...[/dim]")
        rc, out = _run(["docker", "build", "-f", "c++/tools/docker/Dockerfile.fxsdk", "-t", "casio-fxsdk:latest", "."])
        if rc != 0:
            log(f"[bold #f87171]✗ Docker image build failed[/bold #f87171]\n{out}")
            return False
        log("[bold #22c55e]✓[/#22c55e] Docker image ready")
        log("[dim]Running container build (fxsdk build-cg)...[/dim]")
        rc, out = _run([
            "docker", "run", "--rm",
            "-v", f"{REPO}:/work",
            "-w", "/work/c++/addin",
            "casio-fxsdk:latest",
            "bash", "-lc", "fxsdk build-cg"
        ])
        if rc != 0:
            log(f"[bold #f87171]✗ add-in build failed[/bold #f87171]\n{out}")
            return False
    else:
        log("[dim]Building add-in via native fxSDK...[/dim]")
        rc, out = _run([str(native_script)])
        if rc != 0:
            log(f"[bold #f87171]✗ add-in build failed[/bold #f87171]\n{out}")
            return False

    log("[dim]Verifying .g3a output...[/dim]")
    g3as = sorted((REPO / "addin" / "build-cg").glob("*.g3a"))
    if g3as:
        for p in g3as:
            size_kb = p.stat().st_size / 1024
            log(f"[bold #22c55e]✓[/#22c55e] {p.name} ({size_kb:.1f} KB)")
    else:
        log("[bold #f87171]✗ no .g3a found under addin/build-cg/[/bold #f87171]")
        return False
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
        self.backend = "c"
        return AppCls.action_compile_cpp(self)

    AppCls.action_compile = patched_compile  # type: ignore[attr-defined]

    # Launch the TUI app (same as python version).
    app = AppCls()
    app.run()
    AppCls.run_cli = orig  # restore
    AppCls.action_compile = orig_compile
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

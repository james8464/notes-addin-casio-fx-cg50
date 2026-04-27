"""
Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchain.

Run on a PC (Python 3) before copying .mpy to the calculator:
  python3 src/calc_files/check_mpy_build.py

Also copy the small launcher .py files (algebra.py, trig.py, ...) and main.py
from src/calc_files/ along with the .mpy files. Rebuilding *Program.mpy
does not update those launchers; missing run() in algebra.py = silent import.

On-device: optional; needs readable .mpy files in the same folder as this script
or pass paths as arguments.

Expected header (first bytes of .mpy v3, -msmall-int-bits=31, Casio-style build):
  [0] 'M' (0x4D)
  [1] MPY file format version (3 = "v3" bytecode generation)
  [2] feature byte (0x00 with typical mpy-cross flags for this project)
  [3] small-int-bits (0x1F = 31), matching -msmall-int-bits=31
"""

import os
import sys

# Programs shipped as calc_files/<name>.mpy (see tests/run_tests.py action_compile)
EXPECTED_MPY_FILES = (
    "algebraProgram.mpy",
    "trigProgram.mpy",
    "deriveProgram.mpy",
    "intProgram.mpy",
    "SUVATprogram.mpy",
    "booleanProgram.mpy",  # compile target; may be missing until built
    "compat_probe.mpy",
    "main.mpy",
)

EXPECTED_ASCII = 0x4D  # 'M'
EXPECTED_MPY_VERSION = 3
EXPECTED_FEATURE_BYTE = 0x00
EXPECTED_SMALL_INT_BITS = 0x1F  # 31

_RECOMMEND_FLAGS = "-msmall-int-bits=31 -mno-unicode"


def _read_header(path, nbytes=4):
    with open(path, "rb") as f:
        return f.read(nbytes)


def check_one(path):
    out = {"path": path, "ok": False, "reason": ""}
    if not os.path.isfile(path):
        out["reason"] = "missing"
        return out
    data = _read_header(path, 4)
    if len(data) < 4:
        out["reason"] = "too_short"
        return out
    if data[0] != EXPECTED_ASCII:
        out["reason"] = "not_mpy_magic got=0x%02x" % data[0]
        return out
    if data[1] != EXPECTED_MPY_VERSION:
        out["reason"] = "mpy_version want=%s got=%s" % (EXPECTED_MPY_VERSION, data[1])
        return out
    if data[2] != EXPECTED_FEATURE_BYTE:
        out["reason"] = "feature_byte want=0x%02x got=0x%02x" % (
            EXPECTED_FEATURE_BYTE,
            data[2],
        )
        return out
    if data[3] != EXPECTED_SMALL_INT_BITS:
        out["reason"] = "small_int_bits want=0x%02x got=0x%02x" % (
            EXPECTED_SMALL_INT_BITS,
            data[3],
        )
        return out
    out["ok"] = True
    out["reason"] = "ok v%s small_int=%s" % (data[1], data[3])
    return out


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    args = sys.argv[1:]
    if not args:
        paths = [os.path.join(here, name) for name in EXPECTED_MPY_FILES]
    else:
        paths = args

    print("Casio / MicroPython mpy build check")
    print("Expected: magic 'M', mpy format version %s, feature byte 0x%02x, small-int-bits %s" % (
        EXPECTED_MPY_VERSION,
        EXPECTED_FEATURE_BYTE,
        EXPECTED_SMALL_INT_BITS,
    ))
    print("Recommended mpy-cross flags: " + _RECOMMEND_FLAGS)
    print("Use mpy-cross built from MicroPython v1.9.4-era source (bytecode v3).")
    print("")

    bad = 0
    missing = 0
    for p in paths:
        base = os.path.basename(p)
        r = check_one(p)
        if r["reason"] == "missing":
            missing += 1
            print("[--] %s: not found (%s)" % (base, p))
            continue
        if r["ok"]:
            print("[OK] %s: %s" % (base, r["reason"]))
        else:
            bad += 1
            print("[!!] %s: %s" % (base, r["reason"]))

    print("")
    if bad:
        print("Some .mpy files fail the header check; recompile with the matching mpy-cross.")
    if missing:
        print("Some expected files are missing; build from src/Math/*Program.py first.")
    if not bad and not missing:
        print("All present files match the expected header pattern.")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()

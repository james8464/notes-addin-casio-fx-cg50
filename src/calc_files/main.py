# Single entry for calculator: import main; main.run()
# Set _MODULE to the program stem (must match a compiled .mpy you copied).
# ASCII name only. Keep this file very small for the on-device editor.

_MODULE = "trigProgram"


def run():
    try:
        import gc
        gc.collect()
    except Exception:
        pass
    mod = __import__(_MODULE)
    fn = getattr(mod, "run", None) or getattr(mod, "main", None)
    if fn is not None:
        fn()
    else:
        print("Err: no run() or main() in %s" % (_MODULE,))


if __name__ == "__main__":
    run()

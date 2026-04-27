# compat_probe: Casio fx-CG50 MicroPython smoke test (ASCII, uPy-1.9.4 style).
# Run: import compat_probe; compat_probe.run()

try:
    import gc
except ImportError:
    gc = None
try:
    import sys
except ImportError:
    sys = None


def _try_import(label, name):
    try:
        __import__(name)
        print("%s: ok" % (label,))
    except Exception as err:
        print("%s: FAIL %s" % (label, err))


def _try_mpl():
    import matplotlib.pyplot

def run():
    if sys is not None:
        print("sys.version: %s" % (sys.version,))
        print("sys.path: %s" % (sys.path,))
        impl = getattr(sys, "implementation", None)
        if impl is not None:
            print("sys.implementation: %s" % (repr(impl),))
        else:
            print("sys.implementation: (none)")
    if gc is not None:
        for attr in ("mem_free", "mem_alloc"):
            fn = getattr(gc, attr, None)
            if fn is None:
                print("gc.%s: n/a" % (attr,))
                continue
            try:
                print("gc.%s: %s" % (attr, fn()))
            except Exception as err:
                print("gc.%s: err %s" % (attr, err))
    try:
        import micropython
        # mem_info is void; some ports print to stdout
        try:
            micropython.mem_info()
            print("micropython.mem_info: (called)")
        except Exception as err:
            print("micropython.mem_info: err %s" % (err,))
    except Exception as err:
        print("micropython: n/a (%s)" % (err,))

    _try_import("math", "math")
    _try_import("random", "random")
    _try_import("casioplot", "casioplot")
    _try_import("turtle", "turtle")
    try:
        _try_mpl()
        print("matplotlib.pyplot: ok")
    except Exception as err:
        print("matplotlib.pyplot: FAIL %s" % (err,))


if __name__ == "__main__":
    run()

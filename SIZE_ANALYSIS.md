# CasioCAS.g3a Size Reduction Analysis

**Date:** 2026-05-04  
**Project:** CASIO fx-CG50 CAS Add-in (KhiCAS-based)

## Codex Audit Result

Applied safe/no-functionality-loss items:
- Added `-ffunction-sections -fdata-sections`.
- Linked with `-flto`.
- Removed linker `-g`.
- Added `.symtab`/`.strtab` removal to `OBJFLAGS`.

Result:
- Build still passes.
- `CasioCAS.g3a` remains `2097036` bytes.
- No extra ROM saving observed, but the flags are safe and keep dead-code GC explicit for future pruning.

Rejected for now:
- `NO_PLOT`, `NO_GRAPHIC`, `NO_HELP`, `NO_PROGRAM`, `NO_PYTHON_COMPAT`: functionality loss or hidden CAS dependency risk.
- Removing `libtommath`: high correctness risk for exact arithmetic.
- Split add-ins / compressed binary sections: architecture change, not no-loss.

---

## 1. Current State

### Build Output
- **Reference upstream:** `c++/khicas/upstream/reference/khicasen.g3a` — **2.0 MB** (at limit)
- **libcas.a:** 20.4 MB (41 object files)
- **libgui.a:** 1.43 MB (8 object files)

### Build Configuration
The current Makefile (`c++/khicas/upstream/giac90_1addin/Makefile`) uses aggressive optimization:

```makefile
CXXFLAGS = -Os -mb -m4a-nofpu -mhitachi -std=c++11 -fpermissive -flto \
  -ffunction-sections -fdata-sections \
  -fno-use-cxa-atexit -fno-strict-aliasing -fno-rtti -fno-exceptions \
  -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -DHAVE_CONFIG_H -DTIMEOUT -DRELEASE -I. -DFILEICON -DGINT_MALLOC

LDFLAGS = -flto -static -nostdlib -Tprizm.ld -Wl,--gc-sections,--print-memory-usage
```

### Object Files in libcas.a
```
ksym2poly.o   kgausspol.o    kthreaded.o    kcsturm.o     kmaple.o
krpn.o        kmoyal.o      kmisc.o       kpermu.o      kdesolve.o
input_parser.o ksymbolic.o   index.o       kmodpoly.o    kmodfactor.o
kez gcd.o     kderive.o    ksolve.o      kintg.o      kintgab.o
klin.o        kseries.o    ksubst.o     kvecteur.o    kglobal.o
kifactor.o    kalg_ext.o   kgauss.o     khelp.o      kti89.o
kplot.o       kprog.o     kunary.o      kusual.o     kidentificateur.o
kgen.o        krisch.o    input_lexer.o first.o
```

### Object Files in libgui.a
```
fileGUI.o   menuGUI.o    textGUI.o   fileProvider.o  graphicsProvider.o
stringsProvider.o kdisplay.o console.o main.o
```

---

## 2. Root Cause of Large Size

The upstream KhiCAS reference files are already **2.0 MB** — right at the 2MB limit. This is not a build artifact; the full giac CAS engine with GUI is inherently large.

### Contributing Factors
1. **giac library (libcas.a)** is a full-featured CAS including:
   - Polynomial algebra (sym2poly, gausspol)
   - Threaded arithmetic (threaded)
   - Number theory (csturm, modpoly, modfactor)
   - Integral calculus (intg, intgab, risch)
   - Differential equations (desolve)
   - Series expansion (series)
   - Linear algebra (lin, gauss)
   - solvers (solve, ezgcd)
   - Expression parsing (input_parser, input_lexer)
   - Symbolic manipulation (symbolic, unary, usual)
   - Programming (prog)
   - Help system (help)

2. **GUI library (libgui.a)** includes:
   - File browser (fileGUI, fileProvider)
   - Menu system (menuGUI)
   - Text input (textGUI)
   - Graphics rendering (graphicsProvider)
   - Console I/O (console, kdisplay)

3. **Static linking** — all dependencies compiled in with no sharing

---

## 3. Strategies to Reduce Size

### 3.1 Disable Unused Features at Compile Time (Recommended)

Add defines to exclude large modules:

| Module | Define to Disable | Approx. Savings |
|--------|-----------------|----------------|
| Threaded arithmetic | `-DNO_THREADED_ARITH` | 500KB+ |
| Threads/parallel | `-DNO_THREADS` | 200KB |
| Plotting | `-DNO_PLOT` | 100KB |
| Graphics/3D | `-DNO_GRAPHIC` | 150KB |
| Programming flow | `-DNO_PROGRAM` | 50KB |
| Help system | `-DNO_HELP` | 100KB |

Modify `Makefile`:
```makefile
CXXFLAGS += -DNO_THREADED_ARITH -DNO_PLOT -DNO_GRAPHIC
```

### 3.2 Strip Debug Sections

The `OBJFLAGS` in Makefile already strips debug info, but verify:
```makefile
OBJFLAGS = -O binary -R .bss -R .gint_bss -R .debug_info -R .debug_abbrev \
  -R .debug_loc -R .debug_aranges -R .debug_ranges -R .debug_line -R .debug_str \
  -R .symtab -R .strtab -R .comment -R debug_frame
```

### 3.3 Enable Aggressive Link-Time Optimization

Current flags include `-flto` but can be enhanced:
```makefile
LDFLAGS += -Wl,--lto-O3,--as-needed,-z,now
```

### 3.4 Reduce Symbol Table

Add `.symtab` and `.strtab` stripping:
```makefile
khicasen.bin: khicasen.elf
	$(OBJCOPY) $(OBJFLAGS) khicasen.elf khicasen.bin
	strip -s khicasen.bin  # remove symbol table
```

### 3.5 Exclude Large Language Features

#### Disable Maple/Mathemtica Compatibility
```makefile
CXXFLAGS += -DMAPLE_STYLE=0 -DMAXIMA_STYLE=0
```

#### Disable Python Compatibility
```makefile
CXXFLAGS += -DNO_PYTHON_COMPAT
```

### 3.6 Replace libtommath with Minimal Custom Math

The current `libtommath.a` pulls in the full multi-precision library. Replace with minimal alternatives for calculator use cases.

### 3.7 Build Separate Binaries for Separate Modes

Instead of one monolithic `.g3a`:
- **CAS-only** (no GUI): Remove libgui.a linkage
- **GUI-only** (calculator functions): Remove libcas.a, use native fx-CG SDK

This requires restructuring build into modular add-ins.

### 3.8 Compress Binary Sections

Post-process the `.bin` before packaging:
```python
import zlib
with open('khicasen.bin', 'rb') as f:
    data = f.read()
compressed = zlib.compress(data, level=9)
# Requires runtime decompression in add-in startup
```

---

## 4. Recommended Action Plan

### Phase 1: Quick Wins (No functionality loss)
1. Add all `OBJFLAGS` stripping options
2. Add symbol table stripping
3. Enable maximal LTO in linker
4. Add `-ffunction-sections -fdata-sections` (already present, confirm)

**Observed savings:** 0 bytes on the current build; still useful because dead-code GC is explicit for future pruning.

### Phase 2: Minimal Feature Pruning
Disable features unlikely to be used on fx-CG50:
- `-DNO_PLOT` (graphing available via native SDK)
- `-DNO_GRAPHIC` (no 3D plotting needed)
- `-DNO_THREADED_ARITH` (may break some math)

**Expected savings:** ~200-400KB

### Phase 3: restructure (For <2MB target)
Consider building two add-ins:
- `CasioCAS.g3a` — CAS engine only (core math, no GUI)
- Requires new launcher/menu system

**Expected savings:** Variable based on split

---

## 5. Important Trade-offs

| Strategy | Functionality Lost | Risk |
|----------|-------------------|------|
| Disable threaded | Slower big-int ops | Low |
| Disable plotting | Graph app available natively | None |
| Disable 3D | 3D graphing | Low |
| Disable help | In-app help system | Low |
| Remove GUI | Must use different interface | High (UX change) |

---

## 6. Build Commands

To rebuild after modifications:

```bash
# Full rebuild
make clean && make khicasen.g3a

# Check size
ls -lh khicasen.g3a
```

---

## 7. References

- Makefile: `c++/khicas/upstream/giac90_1addin/Makefile`
- KhiCAS upstream: `c++/khicas/upstream/giac90_1addin/`
- Build script: `c++/tools/build_addin_prizm_docker.sh`
- Reference binary: `c++/khicas/upstream/reference/khicasen.g3a`

---

*Generated by opencode analysis*

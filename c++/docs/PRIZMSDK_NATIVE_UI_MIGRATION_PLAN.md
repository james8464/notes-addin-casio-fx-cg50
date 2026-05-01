# PrizmSDK Native UI Migration Plan

This document is a complete offline handoff plan for moving the Casio fx-CG50
add-in UI/input layer from `fxSDK + gint` to a PrizmSDK/libfxcg-style target
that uses the calculator OS syscalls directly.

The intended reader is an AI agent with little Casio add-in experience and no
internet access. Follow the steps in order. Do not skip the verification gates.

## 1. Goal

Build a new `.g3a` target that feels much closer to native Casio apps and
KhiCAS by using the fx-CG OS syscalls exposed by libfxcg:

- real OS text rendering through `Print_OS`, `PrintXY`, `PrintMini`, etc.
- real OS keyboard behavior through `GetKey(&key)`;
- real status/header area through `EnableStatusArea()`, `DefineStatusAreaFlags()`,
  and `DisplayStatusArea()`;
- real `.g3a` packaging through `mkg3a`;
- the existing C++ symbolic/exam-working solver core, unchanged where possible.

The migration should preserve the current `fxSDK + gint` target as a fallback
until the PrizmSDK target is fully proven on hardware.

## 2. Current Repository Facts

Repo root:

```bash
/Users/james/Developer/Python/CASIO
```

Current fxSDK/gint add-in:

```text
c++/addin/CMakeLists.txt
c++/addin/src/main.cpp
c++/addin/src/ui/theme.hpp
c++/addin/src/ui/menu.cpp
c++/addin/src/ui/text_input_device.cpp
c++/tools/build_addin_docker.sh
c++/tools/docker/Dockerfile.fxsdk
```

Current C++ host build:

```text
c++/addin/host/CMakeLists.txt
c++/tools/build_host.sh
```

Current device-safe solver entry point:

```text
c++/addin/src/device/device_solver.hpp
c++/addin/src/device/device_solver.cpp
```

Current solver API to reuse:

```cpp
namespace casio::device
{
enum class Module
{
    Shell,
    Simplify,
    Algebra,
    Derive,
    Integrate,
    Trig,
    Suvat,
};

bool solve(Module module, const char *input, OutputLines &out);
}
```

Current generated fxSDK add-in path:

```text
c++/addin/build-cg/CasioCAS.g3a
```

New PrizmSDK add-in should initially generate:

```text
c++/prizm/build/CasioCAS.g3a
```

Do not delete the fxSDK/gint build until the PrizmSDK target is tested on the
physical calculator.

## 3. Why This Migration Is Needed

The current fxSDK/gint target is a custom UI implementation. It uses gint
display functions such as `dtext()`, `drect()`, `dwindow_set()`, and
`getkey_opt()`. This is powerful, but it is not the same as the Casio OS UI.

Observed limitations:

- the status bar is drawn by our app, not by the OS;
- the clock/status area can only be approximated unless OS syscalls are used;
- text does not match the exact Casio OS text renderer;
- key input approximates native behavior but does not perfectly match every
  shifted/alpha key sequence;
- menu/input widgets are custom, not OS widgets.

The PrizmSDK/libfxcg route exposes OS syscalls, which is the best practical
way to get a native-feeling fx-CG50 add-in.

Important correction: `mkg3a` is not an SDK. It packages a raw binary into
`.g3a`. The native feel comes from libfxcg OS syscalls, not from `mkg3a`.

## 4. Researched Technical Facts To Keep Offline

These facts were checked from the public WikiPrizm/libfxcg/mkg3a sources before
writing this plan.

### 4.1 There Is No Public Official Casio C SDK For fx-CG50

For the fx-CG/Prizm family, the practical C/C++ route is the community
PrizmSDK/libfxcg stack. Casio BASIC is the official user-facing programming
environment, but it is too limited for this symbolic solver add-in.

### 4.2 libfxcg Provides The Native OS Syscall Wrappers

Headers live under:

```text
include/fxcg/
```

Important headers:

```text
include/fxcg/display.h
include/fxcg/keyboard.h
include/fxcg/system.h
include/fxcg/app.h
include/fxcg/rtc.h
```

Important display/status functions from `include/fxcg/display.h`:

```cpp
void Bdisp_AllClr_VRAM(void);
void Bdisp_PutDisp_DD(void);
void Print_OS(const char *msg, int mode, int zero2);
void PrintXY(int x, int y, const char *string, int mode, int color);
void PrintMini(int *x, int *y, const char *MB_string, int mode_flags,
               unsigned int xlimit, int P6, int P7, int color,
               int back_color, int writeflag, int P11);
int DefineStatusAreaFlags(int mode, int flags, char *color1, char *color2);
void DisplayStatusArea(void);
void DrawHeaderLine(void);
void EnableStatusArea(int);
```

Important status constants from `include/fxcg/display.h`:

```cpp
#define DSA_CLEAR          0
#define DSA_SETDEFAULT     1
#define SAF_BATTERY        0x0001
#define SAF_ALPHA_SHIFT    0x0002
#define SAF_SETUP_ANGLE    0x0010
#define SAF_TEXT           0x0100
#define SAF_GLYPH          0x0200
```

Important keyboard functions from `include/fxcg/keyboard.h`:

```cpp
int GetKey(int *key);
int GetKeyWait_OS(int *column, int *row, int type_of_waiting,
                  int timeout_period, int menu, unsigned short *keycode);
```

Important key constants from `include/fxcg/keyboard.h`:

```cpp
#define KEY_CTRL_EXE       30004
#define KEY_CTRL_EXIT      30002
#define KEY_CTRL_MENU      30003
#define KEY_CTRL_DEL       30025
#define KEY_CTRL_AC        30015
#define KEY_CTRL_SHIFT     30006
#define KEY_CTRL_ALPHA     30007
#define KEY_CTRL_LEFT      30020
#define KEY_CTRL_RIGHT     30021
#define KEY_CTRL_UP        30018
#define KEY_CTRL_DOWN      30023
#define KEY_CTRL_F1        30009
#define KEY_CTRL_F2        30010
#define KEY_CTRL_F3        30011
#define KEY_CTRL_F4        30012
#define KEY_CTRL_F5        30013
#define KEY_CTRL_F6        30014

#define KEY_CHAR_LOG       0x95
#define KEY_CHAR_LN        0x85
#define KEY_CHAR_SIN       0x81
#define KEY_CHAR_COS       0x82
#define KEY_CHAR_TAN       0x83
#define KEY_CHAR_SQUARE    0x8b
#define KEY_CHAR_PI        0xd0
#define KEY_CHAR_EXPN      0xa5
#define KEY_CHAR_EXPN10    0xb5
#define KEY_CHAR_ROOT      0x86
#define KEY_CHAR_ASIN      0x91
#define KEY_CHAR_ACOS      0x92
#define KEY_CHAR_ATAN      0x93
```

Important system functions from `include/fxcg/system.h`:

```cpp
void DisplayMainMenu(void);
void PowerOff(int displayLogo);
void OS_InnerWait_ms(int);
```

### 4.3 libfxcg's Build Rules

libfxcg provides:

```text
toolchain/prizm_rules
toolchain/prizm.x
```

The rules expect:

```bash
export FXCGSDK=/path/to/sdk
```

The rules set:

```make
PREFIX := sh3eb-elf-
MACHDEP = -mb -m4a-nofpu -mhitachi -nostdlib
LIBS := -lc -lfxcg -lgcc
```

So the PrizmSDK target should use `sh3eb-elf-gcc` / `sh3eb-elf-g++`, not the
current fxSDK `sh-elf-*` assumptions.

### 4.4 mkg3a Usage

`mkg3a` packages a binary:

```bash
mkg3a -n "basic:CasioCAS" input.bin output.g3a
```

Icon arguments are supported by libfxcg example Makefiles:

```make
MKG3AFLAGS := -n basic:CasioCAS -i uns:assets/unselected.bmp -i sel:assets/selected.bmp
```

## 5. Target Repository Layout

Add a separate PrizmSDK target instead of rewriting the existing fxSDK target:

```text
c++/prizm/
  Makefile
  README.md
  src/
    main_prizm.cpp
    native_ui.cpp
    native_ui.hpp
    native_input.cpp
    native_input.hpp
  assets/
    selected.bmp
    unselected.bmp
  build/
    CasioCAS.g3a

c++/tools/
  build_addin_prizm_docker.sh

c++/tools/docker/
  Dockerfile.prizmsdk

c++/third_party/
  _archives/
    binutils-2.34.tar.bz2
    gcc-10.1.0.tar.xz
    libfxcg-master.tar.gz
    mkg3a-master.tar.gz
```

Keep existing:

```text
c++/addin/
c++/tools/build_addin_docker.sh
c++/tools/docker/Dockerfile.fxsdk
```

## 6. Phase 0: Baseline Before Migration

Run these from repo root:

```bash
cd /Users/james/Developer/Python/CASIO
git status --short
python3 c++/tools/run_tests_cpp.py
./c++/tools/build_addin_docker.sh
python3 run_tests.py /switch c /compile
```

Expected:

```text
python3 c++/tools/run_tests_cpp.py
...
OK

./c++/tools/build_addin_docker.sh
...
Output: /Users/james/Developer/Python/CASIO/c++/addin/build-cg/CasioCAS.g3a

python3 run_tests.py /switch c /compile
...
Compile complete: .g3a ready
```

If these fail, fix the current tree first. Do not start the migration from a
broken baseline.

## 7. Phase 1: Prepare Offline Third-Party Archives

If internet is available, run this once from repo root:

```bash
cd /Users/james/Developer/Python/CASIO
mkdir -p c++/third_party/_archives
rm -rf /tmp/casio_prizm_vendor
mkdir -p /tmp/casio_prizm_vendor

curl -L -o c++/third_party/_archives/binutils-2.34.tar.bz2 \
  http://ftpmirror.gnu.org/binutils/binutils-2.34.tar.bz2

curl -L -o c++/third_party/_archives/gcc-10.1.0.tar.xz \
  http://ftpmirror.gnu.org/gcc/gcc-10.1.0/gcc-10.1.0.tar.xz

git clone --depth=1 https://github.com/Jonimoose/libfxcg.git \
  /tmp/casio_prizm_vendor/libfxcg

git -C /tmp/casio_prizm_vendor/libfxcg rev-parse HEAD \
  > c++/third_party/_archives/libfxcg.commit

tar -C /tmp/casio_prizm_vendor -czf \
  c++/third_party/_archives/libfxcg-master.tar.gz libfxcg

git clone --depth=1 https://github.com/tari/mkg3a.git \
  /tmp/casio_prizm_vendor/mkg3a

git -C /tmp/casio_prizm_vendor/mkg3a rev-parse HEAD \
  > c++/third_party/_archives/mkg3a.commit

tar -C /tmp/casio_prizm_vendor -czf \
  c++/third_party/_archives/mkg3a-master.tar.gz mkg3a

ls -lh c++/third_party/_archives
```

If internet is not available, the AI must check that these files already exist:

```bash
cd /Users/james/Developer/Python/CASIO
ls -lh c++/third_party/_archives/binutils-2.34.tar.bz2
ls -lh c++/third_party/_archives/gcc-10.1.0.tar.xz
ls -lh c++/third_party/_archives/libfxcg-master.tar.gz
ls -lh c++/third_party/_archives/mkg3a-master.tar.gz
```

If any are missing and there is no internet, stop and ask for the missing
archive files. Do not invent replacements.

## 8. Phase 2: Add Dockerfile For PrizmSDK Toolchain

Create:

```text
c++/tools/docker/Dockerfile.prizmsdk
```

Use this content:

```dockerfile
FROM ubuntu:24.04 AS prereqs

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    libgmp-dev \
    libmpc-dev \
    libmpfr-dev \
    libpng-dev \
    make \
    patch \
    pkg-config \
    python3 \
    texinfo \
    xz-utils \
    bzip2 \
    flex \
    && rm -rf /var/lib/apt/lists/*

COPY c++/third_party/_archives/binutils-2.34.tar.bz2 /tmp/binutils-2.34.tar.bz2
COPY c++/third_party/_archives/gcc-10.1.0.tar.xz /tmp/gcc-10.1.0.tar.xz
COPY c++/third_party/_archives/libfxcg-master.tar.gz /tmp/libfxcg-master.tar.gz
COPY c++/third_party/_archives/mkg3a-master.tar.gz /tmp/mkg3a-master.tar.gz

RUN mkdir -p /opt/src /opt/prizm

RUN tar -C /opt/src -xjf /tmp/binutils-2.34.tar.bz2 && \
    mkdir -p /opt/src/build-binutils && \
    cd /opt/src/build-binutils && \
    ../binutils-2.34/configure --target=sh3eb-elf --disable-nls --with-sysroot && \
    make -j"$(nproc)" && \
    make install

RUN tar -C /opt/src -xJf /tmp/gcc-10.1.0.tar.xz && \
    mkdir -p /opt/src/build-gcc && \
    cd /opt/src/build-gcc && \
    ../gcc-10.1.0/configure --target=sh3eb-elf --with-pkgversion=PrizmSDK \
      --without-headers --enable-languages=c,c++ \
      --disable-tls --disable-nls --disable-threads --disable-shared \
      --disable-libssp --disable-libvtv --disable-libada \
      --with-endian=big --with-multilib-list= && \
    make -j"$(nproc)" inhibit_libc=true all-gcc && \
    make install-gcc && \
    make -j"$(nproc)" inhibit_libc=true all-target-libgcc && \
    make install-target-libgcc

RUN tar -C /opt/src -xzf /tmp/mkg3a-master.tar.gz && \
    cmake -S /opt/src/mkg3a -B /opt/src/mkg3a/build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /opt/src/mkg3a/build -j"$(nproc)" && \
    cp /opt/src/mkg3a/build/mkg3a /usr/local/bin/mkg3a && \
    cp /opt/src/mkg3a/build/g3a-updateicon /usr/local/bin/g3a-updateicon || true

RUN tar -C /opt/src -xzf /tmp/libfxcg-master.tar.gz && \
    make -C /opt/src/libfxcg && \
    cp -R /opt/src/libfxcg/include /opt/prizm/include && \
    cp -R /opt/src/libfxcg/lib /opt/prizm/lib && \
    cp -R /opt/src/libfxcg/toolchain /opt/prizm/toolchain && \
    cp -R /opt/src/libfxcg/utils /opt/prizm/utils && \
    mkdir -p /opt/prizm/bin && \
    ln -sf /usr/local/bin/sh3eb-elf-gcc /opt/prizm/bin/sh3eb-elf-gcc && \
    ln -sf /usr/local/bin/sh3eb-elf-g++ /opt/prizm/bin/sh3eb-elf-g++ && \
    ln -sf /usr/local/bin/sh3eb-elf-as /opt/prizm/bin/sh3eb-elf-as && \
    ln -sf /usr/local/bin/sh3eb-elf-ld /opt/prizm/bin/sh3eb-elf-ld && \
    ln -sf /usr/local/bin/sh3eb-elf-objcopy /opt/prizm/bin/sh3eb-elf-objcopy && \
    ln -sf /usr/local/bin/sh3eb-elf-gcc-ar /opt/prizm/bin/sh3eb-elf-gcc-ar && \
    ln -sf /usr/local/bin/mkg3a /opt/prizm/bin/mkg3a

ENV FXCGSDK=/opt/prizm
ENV PATH="/opt/prizm/bin:/usr/local/bin:${PATH}"

WORKDIR /work
```

Build it:

```bash
cd /Users/james/Developer/Python/CASIO
docker build \
  -f c++/tools/docker/Dockerfile.prizmsdk \
  -t casio-prizmsdk:latest \
  .
```

Smoke-check the toolchain:

```bash
docker run --rm casio-prizmsdk:latest bash -lc '
  echo "FXCGSDK=$FXCGSDK"
  sh3eb-elf-gcc --version | head -1
  sh3eb-elf-g++ --version | head -1
  mkg3a -h | head -20
  test -f "$FXCGSDK/lib/libfxcg.a"
  test -f "$FXCGSDK/lib/libc.a"
  test -f "$FXCGSDK/toolchain/prizm_rules"
  test -f "$FXCGSDK/toolchain/prizm.x"
'
```

Expected:

```text
FXCGSDK=/opt/prizm
sh3eb-elf-gcc ...
sh3eb-elf-g++ ...
mkg3a ...
```

## 9. Phase 3: Create The Prizm Target Skeleton

Create directories:

```bash
cd /Users/james/Developer/Python/CASIO
mkdir -p c++/prizm/src c++/prizm/assets c++/prizm/build
```

Copy or create icons:

```bash
cd /Users/james/Developer/Python/CASIO
cp c++/addin/assets/icon-cg-uns.png c++/prizm/assets/unselected.png
cp c++/addin/assets/icon-cg-sel.png c++/prizm/assets/selected.png
```

Important: libfxcg examples use BMP icons. If `mkg3a` rejects PNGs, convert
them to BMP:

```bash
cd /Users/james/Developer/Python/CASIO
python3 - <<'PY'
from PIL import Image
for src, dst in [
    ("c++/prizm/assets/unselected.png", "c++/prizm/assets/unselected.bmp"),
    ("c++/prizm/assets/selected.png", "c++/prizm/assets/selected.bmp"),
]:
    Image.open(src).convert("RGB").save(dst)
PY
```

If Pillow is not installed locally, do the conversion in the existing fxSDK
Docker image because it already installs `python3-pil`:

```bash
cd /Users/james/Developer/Python/CASIO
docker run --rm -v "$PWD:/work" -w /work casio-fxsdk:latest python3 - <<'PY'
from PIL import Image
for src, dst in [
    ("c++/prizm/assets/unselected.png", "c++/prizm/assets/unselected.bmp"),
    ("c++/prizm/assets/selected.png", "c++/prizm/assets/selected.bmp"),
]:
    Image.open(src).convert("RGB").save(dst)
PY
```

## 10. Phase 4: Add `c++/prizm/Makefile`

Create:

```text
c++/prizm/Makefile
```

Use libfxcg's rule style, but explicitly include the current solver sources:

```make
.SUFFIXES:

ifeq ($(strip $(FXCGSDK)),)
export FXCGSDK := /opt/prizm
endif

include $(FXCGSDK)/toolchain/prizm_rules

TARGET := CasioCAS
BUILD := build
SOURCES := src ../addin/src/device ../addin/src/core ../addin/src/modules/algebra ../addin/src/modules/derive ../addin/src/modules/integrate ../addin/src/modules/suvat ../addin/src/modules/trig
DATA :=
INCLUDES := src ../addin/src

MKG3AFLAGS := -n basic:CasioCAS -i uns:assets/unselected.bmp -i sel:assets/selected.bmp

CFLAGS := -Os -Wall -Wextra -Wno-unused-parameter $(MACHDEP) $(INCLUDE) \
  -ffunction-sections -fdata-sections -DTARGET_PRIZM=1 -DCASIO_DEVICE=1

CXXFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions -fno-rtti -fno-use-cxa-atexit

LDFLAGS := $(MACHDEP) -T$(FXCGSDK)/toolchain/prizm.x -Wl,-static -Wl,-gc-sections

LIBS := -lc -lfxcg -lgcc
LIBDIRS :=

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD := $(CXX)

export OFILES := $(addsuffix .o,$(BINFILES)) \
  $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
  $(sFILES:.s=.o) $(SFILES:.S=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
  -I$(CURDIR)/$(BUILD) -I$(LIBFXCG_INC)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(LIBFXCG_LIB)

.PHONY: all clean

all: $(BUILD)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT).bin
	rm -f $(OUTPUT).g3a

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).g3a: $(OUTPUT).bin
$(OUTPUT).bin: $(OFILES)

-include $(DEPENDS)

endif
```

If object filename collisions occur because different source directories have
the same basename, split the source list or rewrite this Makefile to keep object
paths. Do not rename solver files just to satisfy the Makefile.

## 11. Phase 5: Add Minimal Native UI Source Files

Create:

```text
c++/prizm/src/native_ui.hpp
c++/prizm/src/native_ui.cpp
c++/prizm/src/native_input.hpp
c++/prizm/src/native_input.cpp
c++/prizm/src/main_prizm.cpp
```

### 11.1 `native_ui.hpp`

```cpp
#pragma once

#include "device/line_buffer.hpp"

namespace casio::prizm
{

void init_native_screen(const char *title);
void draw_home(void);
void draw_menu(const char *title, const char *const *items, int count, int selected, int top);
void draw_lines(const char *title, casio::device::OutputLines const &lines, int top);
void draw_softkeys(const char *k1, const char *k2, const char *k3,
                   const char *k4, const char *k5, const char *k6);

}
```

### 11.2 `native_ui.cpp`

Start simple. Use OS text first; refine after hardware testing.

```cpp
#include "native_ui.hpp"

#include "device/fixed_string.hpp"

#include <fxcg/display.h>

namespace casio::prizm
{
namespace
{

constexpr int kRows = 8;

void print_line(int row, const char *text, int mode = TEXT_MODE_NORMAL)
{
    PrintXY(1, row, text ? text : "", mode, TEXT_COLOR_BLACK);
}

}

void init_native_screen(const char *title)
{
    Bdisp_AllClr_VRAM();
    EnableStatusArea(1);
    char c1 = 0;
    char c2 = 0;
    DefineStatusAreaFlags(
        DSA_SETDEFAULT,
        SAF_BATTERY | SAF_ALPHA_SHIFT | SAF_SETUP_ANGLE,
        &c1,
        &c2);
    DisplayStatusArea();
    DrawHeaderLine();
    print_line(1, title ? title : "CasioCAS");
}

void draw_softkeys(const char *k1, const char *k2, const char *k3,
                   const char *k4, const char *k5, const char *k6)
{
    PrintXY(1, 8, k1 ? k1 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(7, 8, k2 ? k2 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(13, 8, k3 ? k3 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(19, 8, k4 ? k4 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(25, 8, k5 ? k5 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
    PrintXY(31, 8, k6 ? k6 : "", TEXT_MODE_INVERT, TEXT_COLOR_BLACK);
}

void draw_home(void)
{
    init_native_screen("CasioCAS");
    print_line(2, "EXE: Function Catalog");
    print_line(3, "F1 Shell  F2 Algebra");
    print_line(4, "F3 Deriv  F4 Integr");
    print_line(5, "F5 Trig   F6 SUVAT");
    draw_softkeys("SHELL", "ALG", "DER", "INT", "TRIG", "SUV");
    Bdisp_PutDisp_DD();
}

void draw_menu(const char *title, const char *const *items, int count, int selected, int top)
{
    init_native_screen(title);
    for(int row = 0; row < kRows - 2; row++) {
        int index = top + row;
        if(index >= count) break;
        casio::device::FixedString<64> line;
        line.append_int(index + 1);
        line.append(" ");
        line.append(items[index]);
        print_line(row + 2, line.c_str(), index == selected ? TEXT_MODE_INVERT : TEXT_MODE_NORMAL);
    }
    draw_softkeys("SEL", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

void draw_lines(const char *title, casio::device::OutputLines const &lines, int top)
{
    init_native_screen(title);
    for(int row = 0; row < kRows - 2; row++) {
        int index = top + row;
        if(index >= lines.count()) break;
        print_line(row + 2, lines.line(index));
    }
    draw_softkeys("BACK", "UP", "DOWN", "", "", "EXIT");
    Bdisp_PutDisp_DD();
}

}
```

### 11.3 `native_input.hpp`

```cpp
#pragma once

namespace casio::prizm
{

bool text_input(char *buffer, int capacity, const char *title, const char *help);

}
```

### 11.4 `native_input.cpp`

The first version should use `GetKey(&key)` and Casio key codes. Do not
manually process raw matrix scan codes. The OS should return character key
codes for shifted/alpha functions where available.

```cpp
#include "native_input.hpp"

#include "device/fixed_string.hpp"
#include "native_ui.hpp"

#include <fxcg/display.h>
#include <fxcg/keyboard.h>

namespace casio::prizm
{
namespace
{

int len(const char *s)
{
    int n = 0;
    while(s != nullptr && s[n] != '\0') n++;
    return n;
}

void insert_text(char *buffer, int capacity, int &length, int &cursor, const char *text)
{
    if(text == nullptr) return;
    for(int i = 0; text[i] != '\0'; i++) {
        if(length + 1 >= capacity) break;
        for(int j = length; j >= cursor; j--) buffer[j + 1] = buffer[j];
        buffer[cursor] = text[i];
        length++;
        cursor++;
    }
}

const char *key_to_text(int key)
{
    if(key >= KEY_CHAR_0 && key <= KEY_CHAR_9) {
        static char digit[2];
        digit[0] = (char)key;
        digit[1] = '\0';
        return digit;
    }

    if(key >= KEY_CHAR_A && key <= KEY_CHAR_Z) {
        static char letter[2];
        letter[0] = (char)('a' + (key - KEY_CHAR_A));
        letter[1] = '\0';
        return letter;
    }

    if(key == KEY_CHAR_DP) return ".";
    if(key == KEY_CHAR_PLUS) return "+";
    if(key == KEY_CHAR_MINUS) return "-";
    if(key == KEY_CHAR_MULT) return "*";
    if(key == KEY_CHAR_DIV) return "/";
    if(key == KEY_CHAR_LPAR) return "(";
    if(key == KEY_CHAR_RPAR) return ")";
    if(key == KEY_CHAR_COMMA) return ",";
    if(key == KEY_CHAR_EQUAL) return "=";
    if(key == KEY_CHAR_LOG) return "log(";
    if(key == KEY_CHAR_LN) return "ln(";
    if(key == KEY_CHAR_SIN) return "sin(";
    if(key == KEY_CHAR_COS) return "cos(";
    if(key == KEY_CHAR_TAN) return "tan(";
    if(key == KEY_CHAR_SQUARE) return "^2";
    if(key == KEY_CHAR_POW) return "^";
    if(key == KEY_CHAR_ROOT) return "sqrt(";
    if(key == KEY_CHAR_EXPN) return "exp(";
    if(key == KEY_CHAR_EXPN10) return "10^(";
    if(key == KEY_CHAR_ASIN) return "asin(";
    if(key == KEY_CHAR_ACOS) return "acos(";
    if(key == KEY_CHAR_ATAN) return "atan(";
    if(key == KEY_CHAR_PI) return "pi";
    return nullptr;
}

void draw_input(const char *title, const char *help, const char *buffer)
{
    init_native_screen(title);
    PrintXY(1, 2, help ? help : "", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    PrintXY(1, 4, buffer ? buffer : "", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
    draw_softkeys("sqrt", "log", "sin", "cos", "pi", "=");
    Bdisp_PutDisp_DD();
}

}

bool text_input(char *buffer, int capacity, const char *title, const char *help)
{
    if(buffer == nullptr || capacity <= 0) return false;

    int length = len(buffer);
    if(length >= capacity) {
        length = capacity - 1;
        buffer[length] = '\0';
    }
    int cursor = length;

    draw_input(title, help, buffer);

    while(true) {
        int key = 0;
        GetKey(&key);

        if(key == KEY_CTRL_EXIT) return false;
        if(key == KEY_CTRL_EXE) return true;

        if(key == KEY_CTRL_LEFT) {
            if(cursor > 0) cursor--;
            continue;
        }
        if(key == KEY_CTRL_RIGHT) {
            if(cursor < length) cursor++;
            continue;
        }
        if(key == KEY_CTRL_DEL) {
            if(cursor > 0) {
                for(int i = cursor - 1; i < length; i++) buffer[i] = buffer[i + 1];
                length--;
                cursor--;
                draw_input(title, help, buffer);
            }
            continue;
        }
        if(key == KEY_CTRL_AC) {
            length = 0;
            cursor = 0;
            buffer[0] = '\0';
            draw_input(title, help, buffer);
            continue;
        }

        if(key == KEY_CTRL_F1) insert_text(buffer, capacity, length, cursor, "sqrt(");
        else if(key == KEY_CTRL_F2) insert_text(buffer, capacity, length, cursor, "log(");
        else if(key == KEY_CTRL_F3) insert_text(buffer, capacity, length, cursor, "sin(");
        else if(key == KEY_CTRL_F4) insert_text(buffer, capacity, length, cursor, "cos(");
        else if(key == KEY_CTRL_F5) insert_text(buffer, capacity, length, cursor, "pi");
        else if(key == KEY_CTRL_F6) insert_text(buffer, capacity, length, cursor, "=");
        else insert_text(buffer, capacity, length, cursor, key_to_text(key));

        draw_input(title, help, buffer);
    }
}

}
```

After hardware testing, replace the simple custom editor with OS text routines
if they behave better:

```cpp
DisplayMBString(...)
EditMBStringCtrl(...)
EditMBStringCtrl2(...)
EditMBStringChar(...)
Cursor_SetFlashOn(...)
Keyboard_CursorFlash(...)
```

Those exist in `include/fxcg/display.h` and `include/fxcg/keyboard.h`.

### 11.5 `main_prizm.cpp`

```cpp
#include "device/device_solver.hpp"
#include "device/fixed_string.hpp"
#include "native_input.hpp"
#include "native_ui.hpp"

#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/system.h>

namespace
{

template <int N>
constexpr int count_of(const char *const (&)[N])
{
    return N;
}

int select_menu(const char *title, const char *const *items, int count)
{
    int selected = 0;
    int top = 0;
    casio::prizm::draw_menu(title, items, count, selected, top);

    while(true) {
        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT) return -1;
        if(key == KEY_CTRL_EXE) return selected;
        if(key == KEY_CTRL_UP && selected > 0) selected--;
        if(key == KEY_CTRL_DOWN && selected + 1 < count) selected++;
        if(selected < top) top = selected;
        if(selected >= top + 6) top = selected - 5;
        casio::prizm::draw_menu(title, items, count, selected, top);
    }
}

void view_lines(const char *title, casio::device::OutputLines const &lines)
{
    int top = 0;
    casio::prizm::draw_lines(title, lines, top);
    while(true) {
        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT || key == KEY_CTRL_EXE) return;
        if(key == KEY_CTRL_UP && top > 0) top--;
        if(key == KEY_CTRL_DOWN && top + 6 < lines.count()) top++;
        casio::prizm::draw_lines(title, lines, top);
    }
}

void run_module(const char *title, casio::device::Module module,
                const char *initial, const char *help)
{
    char input[128];
    casio::device::copy_cstr(input, (int)sizeof(input), initial);
    if(!casio::prizm::text_input(input, (int)sizeof(input), title, help)) return;

    casio::device::OutputLines lines;
    casio::device::solve(module, input, lines);
    view_lines(title, lines);
}

}

extern "C" void main(void)
{
    const char *items[] = {
        "CAS Shell",
        "Simplify",
        "Algebra",
        "Derive",
        "Integrate",
        "Trig",
        "SUVAT",
    };

    while(true) {
        casio::prizm::draw_home();

        int key = 0;
        GetKey(&key);
        if(key == KEY_CTRL_EXIT) break;

        int app = -1;
        if(key == KEY_CTRL_F1) app = 0;
        if(key == KEY_CTRL_F2) app = 2;
        if(key == KEY_CTRL_F3) app = 3;
        if(key == KEY_CTRL_F4) app = 4;
        if(key == KEY_CTRL_F5) app = 5;
        if(key == KEY_CTRL_F6) app = 6;
        if(key == KEY_CTRL_EXE) app = select_menu("Home", items, count_of(items));
        if(app < 0) continue;

        switch(app) {
            case 0:
                run_module("CAS Shell", casio::device::Module::Shell, "2x+3=7", "expr, eqn, diff(...), int(...)");
                break;
            case 1:
                run_module("Simplify", casio::device::Module::Simplify, "2x+3-x+4", "linear expression");
                break;
            case 2:
                run_module("Algebra", casio::device::Module::Algebra, "2x+3=7", "linear equation");
                break;
            case 3:
                run_module("Derive", casio::device::Module::Derive, "3x^2+2x+1", "polynomial up to x^5");
                break;
            case 4:
                run_module("Integrate", casio::device::Module::Integrate, "3x^2+2x+1", "polynomial up to x^5");
                break;
            case 5:
                run_module("Trig", casio::device::Module::Trig, "sin(30)", "exact common angles");
                break;
            case 6:
                run_module("SUVAT", casio::device::Module::Suvat, "s=10,u=0,v=?,a=2,t=5", "use ? for target");
                break;
            default:
                break;
        }
    }
}
```

## 12. Phase 6: Add Build Script

Create:

```text
c++/tools/build_addin_prizm_docker.sh
```

Use this content:

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_TAG="casio-prizmsdk:latest"

echo "=== Building PrizmSDK Docker image ==="
docker build \
  -f "${ROOT_DIR}/c++/tools/docker/Dockerfile.prizmsdk" \
  -t "${IMAGE_TAG}" \
  "${ROOT_DIR}"

echo ""
echo "=== Removing stale Prizm outputs ==="
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.g3a"
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.bin"
rm -rf "${ROOT_DIR}/c++/prizm/build"

echo ""
echo "=== Building native Prizm add-in ==="
docker run --rm \
  -v "${ROOT_DIR}:/work" \
  -w /work/c++/prizm \
  "${IMAGE_TAG}" \
  bash -lc '
    echo "FXCGSDK=$FXCGSDK"
    sh3eb-elf-g++ --version | head -1
    mkg3a -h | head -5
    make clean
    make -j"$(nproc)"
  '

echo ""
echo "=== Build Results ==="
if [[ -f "${ROOT_DIR}/c++/prizm/CasioCAS.g3a" ]]; then
  mkdir -p "${ROOT_DIR}/c++/prizm/build"
  mv "${ROOT_DIR}/c++/prizm/CasioCAS.g3a" "${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a"
fi

if [[ -f "${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a" ]]; then
  ls -lh "${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a"
  echo "Output: ${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a"
else
  echo "No Prizm .g3a produced"
  exit 1
fi
```

Make it executable:

```bash
cd /Users/james/Developer/Python/CASIO
chmod +x c++/tools/build_addin_prizm_docker.sh
```

Build:

```bash
cd /Users/james/Developer/Python/CASIO
./c++/tools/build_addin_prizm_docker.sh
```

Expected output:

```text
Output: /Users/james/Developer/Python/CASIO/c++/prizm/build/CasioCAS.g3a
```

## 13. Phase 7: Resolve Expected Compile Issues

### 13.1 Missing C++ Runtime Symbols

If link fails with C++ runtime symbols such as:

```text
__cxa_atexit
__gxx_personality_v0
operator new
operator delete
```

Do this:

1. Confirm `-fno-exceptions`, `-fno-rtti`, and `-fno-use-cxa-atexit` are in
   `CXXFLAGS`.
2. Search for accidental STL usage in the Prizm target:

```bash
cd /Users/james/Developer/Python/CASIO
rg -n "std::|#include <string>|#include <vector>|#include <memory>|throw|new |delete " c++/prizm c++/addin/src
```

3. If `operator new/delete` are still required, add a tiny device allocator
   file only if needed:

```text
c++/prizm/src/device_new.cpp
```

```cpp
#include <stddef.h>

extern "C" void *malloc(size_t);
extern "C" void free(void *);

void *operator new(size_t size) { return malloc(size); }
void *operator new[](size_t size) { return malloc(size); }
void operator delete(void *ptr) noexcept { free(ptr); }
void operator delete[](void *ptr) noexcept { free(ptr); }
void operator delete(void *ptr, size_t) noexcept { free(ptr); }
void operator delete[](void *ptr, size_t) noexcept { free(ptr); }
```

Avoid this if the link succeeds without it.

### 13.2 Duplicate Object Basenames

If the Makefile reports duplicate `.o` targets because two `.cpp` files share
the same basename, use one of these fixes:

Preferred:

- replace the generic libfxcg Makefile with explicit object paths;
- keep each object under `build/path/to/source.o`.

Temporary:

- split `SOURCES` into smaller targets;
- rename only Prizm wrapper files, not shared solver files.

### 13.3 Missing Math Functions

The device solver currently avoids most floating math in the device path. If
the linker complains about `sin`, `cos`, `pow`, etc., do not add OpenLibm
blindly. First confirm the offending source is included intentionally.

Search:

```bash
cd /Users/james/Developer/Python/CASIO
rg -n "sin\\(|cos\\(|tan\\(|pow\\(|sqrt\\(|log\\(|exp\\(" c++/addin/src c++/prizm/src
```

If needed, use integer/exact alternatives already in the solver.

## 14. Phase 8: Integrate With The Test TUI

Current `/compile` builds:

```text
./c++/tools/build_host.sh
./c++/tools/build_addin_docker.sh
```

Add a new environment switch first. Do not replace the fxSDK build immediately.

Desired behavior:

```bash
CASIO_BUILD_ADDIN=prizm python3 run_tests.py /switch c /compile
```

Implementation steps:

1. Open:

```text
python/tests/run_tests.py
```

2. Find where `CASIO_BUILD_ADDIN` is read.

```bash
cd /Users/james/Developer/Python/CASIO
rg -n "CASIO_BUILD_ADDIN|build_addin" python/tests/run_tests.py
```

3. Extend the logic:

```python
addin_mode = os.environ.get("CASIO_BUILD_ADDIN", "docker").lower()
if addin_mode == "prizm":
    addin_script = REPO_ROOT / "c++" / "tools" / "build_addin_prizm_docker.sh"
elif addin_mode == "docker":
    addin_script = REPO_ROOT / "c++" / "tools" / "build_addin_docker.sh"
elif addin_mode in ("0", "false", "off", "none", "skip"):
    addin_script = None
else:
    # print clear unsupported mode error
```

4. Update progress text to say which add-in target is being built:

```text
--- Building add-in (.g3a, PrizmSDK/native OS) ---
```

5. Verify:

```bash
cd /Users/james/Developer/Python/CASIO
CASIO_BUILD_ADDIN=prizm python3 run_tests.py /switch c /compile
CASIO_BUILD_ADDIN=docker python3 run_tests.py /switch c /compile
```

Acceptance:

- `CASIO_BUILD_ADDIN=prizm` produces `c++/prizm/build/CasioCAS.g3a`;
- `CASIO_BUILD_ADDIN=docker` still produces `c++/addin/build-cg/CasioCAS.g3a`.

## 15. Phase 9: Hardware Smoke Test Checklist

Copy:

```text
c++/prizm/build/CasioCAS.g3a
```

to the calculator.

On the calculator:

1. Confirm the menu icon is visible and named `CasioCAS`.
2. Launch the add-in.
3. Confirm the top status area looks like the calculator OS, not our custom
   drawn bar.
4. Wait 60 seconds on the home screen.
5. Confirm the status area clock/battery area updates normally.
6. Press `SHIFT`.
7. Confirm the status area shows Shift using the OS indicator.
8. Press `ALPHA`.
9. Confirm the status area shows Alpha using the OS indicator.
10. Press `SHIFT`, then `ln`.
11. Confirm the returned key becomes the OS shifted value, not plain `ln`.
12. Press `log`, `ln`, `sin`, `cos`, `tan`, `x^2`, `sqrt`, `pi`, `EXP`.
13. Confirm the input text matches calculator-native expectations.
14. Run algebra:

```text
2x+3=7
```

Expected output includes:

```text
x = 2
```

15. Run derive:

```text
3x^2+2x+1
```

Expected output includes:

```text
dy/dx = 6*x + 2
```

16. Run integrate:

```text
3x^2+2x+1
```

Expected output includes:

```text
x^3 + x^2 + x + C
```

17. Press `MENU` while in the add-in. Confirm OS behavior is sane.
18. Return to the add-in. Confirm screen redraws correctly.
19. Press `EXIT` from each view. Confirm navigation is sane.

Record hardware notes in:

```text
c++/prizm/HARDWARE_TEST_NOTES.md
```

## 16. Phase 10: Improve Native Input After First Hardware Test

The first Prizm input editor may still be too simple. After smoke testing,
upgrade it with OS editor syscalls.

Candidate libfxcg functions:

```cpp
void locate_OS(int X, int y);
void Cursor_SetFlashOn(unsigned char cursor_type);
void Cursor_SetFlashOff(void);
int SetCursorFlashToggle(int);
void Keyboard_CursorFlash(void);
void DisplayMBString(unsigned char *MB_string, int start, int xpos, int x, int y);
void EditMBStringCtrl(unsigned char *MB_string, int posmax, int *start, int *xpos,
                      int *key, int x, int y);
void EditMBStringCtrl2(unsigned char *MB_string, int xposmax, int *P3, int *xpos,
                       int *key, int x, int y, int enable_pos_to_clear,
                       int pos_to_clear);
int EditMBStringChar(unsigned char *MB_string, int posmax, int xpos, int char_to_insert);
```

Preferred evolution:

1. Keep `GetKey(&key)` as the only key source.
2. Use `EditMBStringChar()` for insertion if it handles multi-byte native tokens
   better than our string insertion.
3. Use `DisplayMBString()` to draw input.
4. Use `Cursor_SetFlashOn()` and `Keyboard_CursorFlash()` for cursor.
5. Keep parser-facing text ASCII by converting OS tokens to solver strings:

```text
KEY_CHAR_ROOT   -> sqrt(
KEY_CHAR_EXPN   -> exp(
KEY_CHAR_EXPN10 -> 10^(
KEY_CHAR_PI     -> pi
```

## 17. Phase 11: Decide When To Retire fxSDK/gint

Do not remove fxSDK/gint until all criteria are met:

- PrizmSDK `.g3a` builds repeatably in Docker.
- PrizmSDK `.g3a` launches on hardware.
- Native status area is visible and live.
- Shift/Alpha behavior matches calculator OS behavior better than gint.
- Text rendering visibly matches native OS text better than `dtext()`.
- All core C++ tests pass.
- `/compile` can build the Prizm target through `CASIO_BUILD_ADDIN=prizm`.
- User confirms the hardware feel is closer to KhiCAS/default apps.

When all are true:

1. Change TUI default:

```bash
CASIO_BUILD_ADDIN=prizm
```

2. Keep old fxSDK/gint script as fallback:

```bash
CASIO_BUILD_ADDIN=docker
```

3. Update docs:

```text
c++/addin/README.md
c++/addin/DEVICE_SMOKE_CHECKLIST.md
c++/prizm/README.md
README.md
```

## 18. Verification Commands After Each Implementation Pass

Run these after every meaningful edit:

```bash
cd /Users/james/Developer/Python/CASIO
git status --short
python3 c++/tools/run_tests_cpp.py
./c++/tools/build_addin_prizm_docker.sh
CASIO_BUILD_ADDIN=prizm python3 run_tests.py /switch c /compile
python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"
```

If the old fxSDK target was touched, also run:

```bash
./c++/tools/build_addin_docker.sh
CASIO_BUILD_ADDIN=docker python3 run_tests.py /switch c /compile
```

## 19. Git Commit Strategy

Make small commits at working checkpoints:

```bash
git status --short
git add <changed files>
git commit -m "Add PrizmSDK toolchain plan"
```

Suggested future commits:

```text
Vendor PrizmSDK build archives
Add PrizmSDK Docker toolchain
Add native Prizm add-in skeleton
Wire Prizm target into compile command
Use OS input editor for Prizm target
Document Prizm hardware smoke results
```

## 20. Rollback Plan

If the PrizmSDK path fails:

1. Leave the current fxSDK/gint path untouched.
2. Keep using:

```bash
./c++/tools/build_addin_docker.sh
python3 run_tests.py /switch c /compile
```

3. Remove only the new Prizm files if necessary:

```bash
rm -rf c++/prizm
rm -f c++/tools/build_addin_prizm_docker.sh
rm -f c++/tools/docker/Dockerfile.prizmsdk
```

4. Do not delete shared solver files under:

```text
c++/addin/src/core
c++/addin/src/modules
c++/addin/src/device
```

## 21. Final Acceptance Criteria

The migration is complete only when all are true:

- `python3 c++/tools/run_tests_cpp.py` passes.
- `./c++/tools/build_addin_prizm_docker.sh` exits 0.
- `CASIO_BUILD_ADDIN=prizm python3 run_tests.py /switch c /compile` exits 0.
- `c++/prizm/build/CasioCAS.g3a` exists.
- The `.g3a` has a visible icon and app name on the calculator.
- The top status area is OS-native, not manually drawn.
- The OS Shift/Alpha indicators appear in the status area.
- `GetKey(&key)` produces shifted/alpha character codes matching the physical
  keyboard labels.
- OS text routines visibly match the default calculator apps better than the
  old `dtext()` UI.
- Solver outputs still show exam-quality working lines.


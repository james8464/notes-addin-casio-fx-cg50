.SUFFIXES:

ifeq ($(strip $(FXCGSDK)),)
export FXCGSDK := /opt/prizm
endif

include $(FXCGSDK)/toolchain/prizm_rules

TARGET := CasioCAS
BUILD := build
SOURCES := src ../addin/src/device ../addin/src/core ../addin/src/core
DATA :=
INCLUDES := src ../addin/src

# Icon paths relative to build directory ($(OUTPUT) is absolute)
MKG3AFLAGS := -n basic:CasioCAS -i uns:../assets/unselected.bmp -i sel:../assets/selected.bmp

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

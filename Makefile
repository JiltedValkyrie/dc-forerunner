# <project> Dreamcast port. Needs the KOS environment (tools/build.ps1 sets it up from config/config.local). No literal paths here.
# Dependencies: python tools/fresh_deps.py --ensure clones SH4ZAM, SH4DER-PVR, SH4DOW-DC at their pins into third_party/ (git-ignored).
# PROJECT names the ELF (build/$(PROJECT).elf). build.ps1 passes PORT_NAME from config/config.local.
ifndef KOS_BASE
$(error KOS_BASE not set. Use tools/build.ps1)
endif
PROJECT ?= port
SH4ZAM    ?= third_party/sh4zam
SH4DER    ?= third_party/sh4der-pvr
SH4DOW    ?= third_party/sh4dow-dc
SH4ZAM_LIB = $(SH4ZAM)/build/libsh4zam.a
MATHFLAGS = -I$(SH4ZAM)/include -ffast-math -ffp-contract=fast -mfsrra -mfsca -fno-math-errno
# USE_SH4DOW=1 / USE_SH4DER=1: build and link the effect libraries (static archives, only the used parts are pulled in). Off by default: the title screen uses neither.
USE_SH4DOW ?= 0
USE_SH4DER ?= 0
LIBS = $(SH4ZAM_LIB)
CFLAGS_ALL = -O2 -Wall -Wextra $(MATHFLAGS)
ifeq ($(USE_SH4DOW),1)
CFLAGS_ALL += -I$(SH4DOW)/include
SH4DOW_SRCS = $(wildcard $(SH4DOW)/src/*.c)
LIBS += build/libsh4dow_dc.a
endif
ifeq ($(USE_SH4DER),1)
CFLAGS_ALL += -I$(SH4DER)/include -I$(SH4DOW)/include   # SH4DER-PVR builds on SH4DOW-DC
SH4DER_SRCS = $(wildcard $(SH4DER)/src/*.c)
LIBS += build/libsh4der_pvr.a
endif
TARGET = build/$(PROJECT).elf
OBJS = build/main.o
all: $(TARGET)
build/%.o: src/%.c
	@mkdir -p build
	kos-cc $(CFLAGS_ALL) -c $< -o $@
build/lib/sh4dow/%.o: $(SH4DOW)/src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS_ALL) -c $< -o $@
build/lib/sh4der/%.o: $(SH4DER)/src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS_ALL) -c $< -o $@
build/libsh4dow_dc.a: $(patsubst $(SH4DOW)/src/%.c,build/lib/sh4dow/%.o,$(SH4DOW_SRCS))
	rm -f $@ && sh-elf-ar rcs $@ $^
build/libsh4der_pvr.a: $(patsubst $(SH4DER)/src/%.c,build/lib/sh4der/%.o,$(SH4DER_SRCS))
	rm -f $@ && sh-elf-ar rcs $@ $^
$(TARGET): $(OBJS) $(LIBS)
	kos-cc -o $@ $(OBJS) $(LIBS)
clean:
	rm -rf build/*.o build/lib build/*.a $(TARGET)
-include tools/sh4zam_lib.mk

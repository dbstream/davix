# Main kernel makefile
# Copyright (C) 2024  dbstream

# Define the kernel version
# This is simply YYYY.MM.DD.N, the date when I last bothered to change this. We
# will probably move to some kind of major-minor-patch versioning scheme once
# userspace gets going.
KERNELVERSION	:= 2025.02.18.1
export KERNELVERSION

ifeq ($(obj),)

# Silence "make [n]: Entering subdirectory..." and remove default rules.
MAKEFLAGS += -R -r --no-print-directory

ifneq ($(abspath $(dir $(lastword $(MAKEFILE_LIST)))),$(CURDIR))
$(error Out-of-tree invocations are not supported.)
endif

ifeq ("$(origin O)","command line")
objtree := $(O)
else
objtree := build
endif

export objtree

# Silence commands by default, unless V=1.
Q := @
ifeq ("$(origin V)","command line")
ifneq ($(filter-out 0 n N,$(V)),)
Q :=
endif
endif

export Q

# Forward our invocation to build.GNUmakefile.
.PHONY: $(or $(filter-out clean,$(MAKECMDGOALS)),all)
$(or $(filter-out clean,$(MAKECMDGOALS)),all):
	$(Q)mkdir -p -- $(objtree)
	$(Q)$(MAKE) -f build.GNUmakefile subdir= $(MAKECMDGOALS)

ifeq ($(objtree),build)
.PHONY: clean
clean:
	$(Q)if [ -d build ]; then rm -r $(objtree); fi
endif

else

# Detect the architecture we are compiling for. Use user-supplied
# $(ARCH) if it exists, otherwise query utsname.
ifeq ($(ARCH),)
override ARCH := $(shell uname -m)
endif

# x86_64 means x86.
ifeq ($(ARCH),x86_64)
override ARCH := x86
endif

CPPFLAGS :=					\
	-Iinclude				\
	-Iarch/$(ARCH)/include			\
	-Iuser					\
	-include include/builtin_config.h
CFLAGS :=					\
	-Wall					\
	-Wextra					\
	-Werror					\
	-O2					\
	-g					\
	-ffreestanding				\
	-fvisibility=hidden			\
	-nostdlib				\
	-nostdinc				\
	-std=gnu17
AFLAGS :=

include arch/$(ARCH)/GNUmakefile

export CC AS LD OBJCOPY CPPFLAGS CFLAGS AFLAGS

kobjs += acpi/
kobjs += arch/$(ARCH)/
kobjs += drivers/
kobjs += fs/
kobjs += kernel/
kobjs += ktest/
kobjs += mm/
kobjs += util/

all: $(obj)init

targets += $(obj)init.o $(obj)init
$(obj)init.o: cflags=$(CPPFLAGS) $(CFLAGS-init)
$(obj)init: ldflags=$(LDFLAGS-init)
$(obj)init: ldlibs=$(LDLIBS-init)
$(obj)init: $(obj)init.o FORCE
	$(call cond_cmd,ld)

# Up to architecture to define an $(obj)davix.
targets += $(obj)davix.elf
$(obj)davix.elf: ldflags=$(LDFLAGS-davix)
$(obj)davix.elf: ldlibs=$(LDLIBS-davix)
$(obj)davix.elf: $(LDSCRIPT-davix) $(obj)kernel.a FORCE
	$(call cond_cmd,ld)

endif
